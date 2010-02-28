
#include <ctype.h>

#include "scenario.h"
#include "system.h"


typedef struct setting_t {

    char *                  name;
    char *                  value;

    struct setting_t **     setting_list;
    uint16                  setting_count;

    struct setting_t *      parent_setting;

} setting_t;


    /**** global variables ****/

static char                 error_string[256];


    /**** local function prototypes ****/

setting_t *                 setting_create(char *name, setting_t *parent_setting);
void                        setting_destroy(setting_t *setting);
void                        setting_set_value(setting_t *setting, char *value);
void                        setting_add_subsetting(setting_t *setting, setting_t *subsetting);

bool                        fprint_setting_tree(FILE *f, setting_t *setting, uint16 level);
bool                        parse_setting_tree(setting_t *setting, char *path, node_t *node);
setting_t *                 create_setting_tree();

bool                        apply_system_setting(char *path, char *name, char *value);
bool                        apply_node_setting(char *path, node_t *node, char *name, char *value);

bool                        apply_phy_setting(char *path, phy_node_info_t *phy_node_info, char *name, char *value);


    /**** exported functions ****/

char *scenario_load(char *filename)
{
    setting_t *root_setting = setting_create("scenario", NULL);
    setting_set_value(root_setting, filename);

    setting_t *current_setting = NULL;
    setting_t *parent_setting = root_setting;

    error_string[0] = '\0';

    rs_debug(DEBUG_SCENARIO, "loading scenario file '%s'", filename);

    FILE *f = fopen(filename, "r");

    if (f == NULL) {
        sprintf(error_string, "failed to open '%s': %s", filename, strerror(errno));
        return error_string;
    }

    char state = 'n'; /* 'n'ame, 'v'alue */
    char name[256];
    char value[256];

    name[0] = '\0';
    value[0] = '\0';

    int c;
    while ((c = fgetc(f)) != EOF) {
        if (isspace(c)) { /* ignore spaces */
            continue;
        }

        if (c == '=') {
            if (state != 'n' || strlen(name) == 0 || current_setting != NULL) {
                sprintf(error_string, "unexpected '%c'", c);
                break;
            }

            current_setting = setting_create(name, parent_setting);
            name[0] = '\0';

            state = 'v';
        }
        else if (c == '{') {
            if (state != 'v' || strlen(value) > 0) {
                sprintf(error_string, "unexpected '%c'", c);
                break;
            }

            parent_setting = current_setting;
            current_setting = NULL;

            state = 'n';
        }
        else if (c == ';') {
            if (state != 'v') {
                sprintf(error_string, "unexpected '%c'", c);
                break;
            }

            setting_set_value(current_setting, value);
            value[0] = 0;

            current_setting = NULL;

            state = 'n';
        }
        else if (c == '}') {
            if (state != 'n' || parent_setting->parent_setting == NULL) {
                sprintf(error_string, "unexpected '%c'", c);
                break;
            }

            current_setting = parent_setting;
            parent_setting = parent_setting->parent_setting;

            current_setting = NULL;
        }
        else if (isalnum(c) || c == '_' || c == '.') {
            if (state == 'n') {
                name[strlen(name) + 1] = '\0';
                name[strlen(name)] = c;
            }
            else if (state == 'v') {
                value[strlen(value) + 1] = '\0';
                value[strlen(value)] = c;
            }
            else {
                sprintf(error_string, "unexpected '%c'", c);
                break;
            }
        }
        else {
            sprintf(error_string, "unexpected '%c'", c);
            break;
        }
    }

    fclose(f);

    if (current_setting != NULL && strlen(error_string) == 0) {
        strcpy(error_string, "unexpected end of file");
    }

    if (strlen(error_string) > 0) {
        setting_destroy(root_setting);

        return error_string;
    }
    else {
        bool all_ok = parse_setting_tree(root_setting, "", NULL);
        setting_destroy(root_setting);

        if (!all_ok) {
            return error_string;
        }
        else {
            return NULL;
        }
    }
}

char* scenario_save(char *filename)
{
    FILE *f = fopen(filename, "w");

    if (f == NULL) {
        sprintf(error_string, "failed to open '%s': %s", filename, strerror(errno));
        return error_string;
    }

    setting_t *root_setting = create_setting_tree();

    uint16 i;
    bool all_ok = TRUE;
    for (i = 0; i < root_setting->setting_count; i++) {
        all_ok = fprint_setting_tree(f, root_setting->setting_list[i], 0);
        if (!all_ok) {
            break;
        }
    }

    fprintf(f, "\n");
    fclose(f);

    if (!all_ok) {
        return error_string;
    }
    else {
        return NULL;
    }
}


    /**** local functions ****/

setting_t *setting_create(char *name, setting_t *parent_setting)
{
    setting_t *setting = malloc(sizeof(setting_t));

    setting->name = strdup(name);
    setting->value = NULL;
    setting->setting_list = NULL;
    setting->setting_count = 0;
    setting->parent_setting = parent_setting;

    if (parent_setting != NULL) {
        setting_add_subsetting(parent_setting, setting);
    }

    return setting;
}

void setting_destroy(setting_t *setting)
{
    rs_assert(setting != NULL);

    if (setting->name != NULL) {
        free(setting->name);
    }

    if (setting->setting_list != NULL) {
        uint16 i;
        for (i = 0; i < setting->setting_count; i++) {
            setting_destroy(setting->setting_list[i]);
        }

        free(setting->setting_list);
    }

    free(setting);
}

void setting_set_value(setting_t *setting, char *value)
{
    rs_assert(setting != NULL);

    if (setting->value != NULL) {
        free(setting->value);
    }

    setting->value = strdup(value);
}

void setting_add_subsetting(setting_t *setting, setting_t *subsetting)
{
    rs_assert(setting != NULL);

    setting->setting_list = realloc(setting->setting_list, (setting->setting_count + 1) * sizeof(setting_t *));
    setting->setting_list[setting->setting_count++] = subsetting;
}

bool fprint_setting_tree(FILE *f, setting_t *setting, uint16 level)
{
    rs_assert(setting != NULL);

    char indent[256];
    uint16 i;
    indent[0] = '\0';
    for (i = 0; i < level; i++) {
        strcat(indent, "    ");
    }

    if (setting->value != NULL) {
        fprintf(f, "%s%s = %s;\n", indent, setting->name, setting->value);
        return TRUE;
    }
    else {
        fprintf(f, "%s%s = {\n", indent, setting->name);

        uint16 i;
        bool all_ok = TRUE;
        for (i = 0; i < setting->setting_count; i++) {
            if (!fprint_setting_tree(f, setting->setting_list[i], level + 1)) {
                all_ok = FALSE;
                break;
            }
        }

        fprintf(f, "%s}\n", indent);

        return all_ok;
    }
}

bool parse_setting_tree(setting_t *setting, char *path, node_t *node)
{
    rs_assert(setting != NULL);

    if (setting->setting_count == 0) {
        if (setting->parent_setting == NULL) {
            return TRUE;
        }

        if (strcmp(setting->parent_setting->name, "system") == 0) {
            return apply_system_setting(path, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "node") == 0) {
            return apply_node_setting(path, node, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "phy") == 0) {
            return apply_phy_setting(path, node->phy_info, setting->name, setting->value);
        }
        else {
            sprintf(error_string, "unexpected setting '%s.%s'", path, setting->name);
            return FALSE;
        }
    }
    else {
        if (strcmp(setting->name, "system") == 0) {
            /* nothing */
        }
        else if (strcmp(setting->name, "node") == 0) {
            node = node_create();

            measure_node_init(node);
            phy_node_init(node, "", 0, 0);
            mac_node_init(node, "");
            ip_node_init(node, "");
            icmp_node_init(node);
            rpl_node_init(node);

            rs_system_add_node(node);
        }
        else if (strcmp(setting->name, "phy") == 0) {
            /* nothing */
        }

        char new_path[256];
        if (strlen(path) > 0) {
            sprintf(new_path, "%s.%s", path, setting->name);
        }
        else {
            sprintf(new_path, "%s", setting->name);
        }

        uint16 i;
        bool all_ok = TRUE;
        for (i = 0; i < setting->setting_count; i++) {
            if (!parse_setting_tree(setting->setting_list[i], new_path, node)) {
                all_ok = FALSE;
                break;
            }
        }

        return all_ok;
    }
}

setting_t *create_setting_tree()
{
    setting_t *setting;
    char text[256];

    setting_t *root_setting = setting_create("scenario", NULL);
    setting_t *system_setting = setting_create("system", root_setting);

    setting = setting_create("no_link_dist_thresh", system_setting);
    sprintf(text, "%.02f", rs_system->no_link_dist_thresh);
    setting_set_value(setting, text);

    setting = setting_create("no_link_quality_thresh", system_setting);
    sprintf(text, "%.02f", rs_system->no_link_quality_thresh);
    setting_set_value(setting, text);

    uint16 i, node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];

        setting_t *node_setting = setting_create("node", system_setting);

        setting_t *phy_setting = setting_create("phy", node_setting);

        setting = setting_create("name", phy_setting);
        setting_set_value(setting, node->phy_info->name);

        setting = setting_create("cx", phy_setting);
        sprintf(text, "%.02f", node->phy_info->cx);
        setting_set_value(setting, text);

        setting = setting_create("cy", phy_setting);
        sprintf(text, "%.02f", node->phy_info->cy);
        setting_set_value(setting, text);
    }

    return root_setting;
}

bool apply_system_setting(char *path, char *name, char *value)
{
    if (strcmp(name, "no_link_dist_thresh") == 0) {
        rs_system->no_link_dist_thresh = strtof(value, NULL);
    }
    else if (strcmp(name, "no_link_quality_thresh") == 0) {
        rs_system->no_link_quality_thresh = strtof(value, NULL);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_node_setting(char *path, node_t *node, char *name, char *value)
{
    /* no separate node setting */
    sprintf(error_string, "unexpected setting '%s.%s'", path, name);

    return FALSE;
}

bool apply_phy_setting(char *path, phy_node_info_t *phy_node_info, char *name, char *value)
{
    if (strcmp(name, "name") == 0) {
        phy_node_info->name = strdup(value);
    }
    else if (strcmp(name, "cx") == 0) {
        phy_node_info->cx = strtof(value, NULL);
    }
    else if (strcmp(name, "cy") == 0) {
        phy_node_info->cy = strtof(value, NULL);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}
