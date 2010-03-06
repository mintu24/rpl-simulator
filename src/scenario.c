
#include <ctype.h>

#include "scenario.h"
#include "system.h"
#include "gui/mainwin.h"


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
bool                        load_post_process();

bool                        parse_setting_tree(setting_t *setting, char *path, node_t *node);
setting_t *                 create_setting_tree();

bool                        apply_system_setting(char *path, char *name, char *value);
bool                        apply_display_setting(char *path, char *name, char *value);
bool                        apply_events_setting(char *path, char *name, char *value);

bool                        apply_phy_setting(char *path, phy_node_info_t *phy_node_info, char *name, char *value);
bool                        apply_mobility_setting(char *path, phy_mobility_t *mobility, char *name, char *value);

bool                        apply_mac_setting(char *path, mac_node_info_t *mac_node_info, char *name, char *value);

bool                        apply_ip_setting(char *path, ip_node_info_t *ip_node_info, char *name, char *value);
bool                        apply_route_setting(char *path, ip_route_t *route, char *name, char *value);

bool                        apply_icmp_setting(char *path, icmp_node_info_t *icmp_node_info, char *name, char *value);

bool                        apply_rpl_setting(char *path, rpl_node_info_t *rpl_node_info, char *name, char *value);

bool                        apply_measure_setting(char *path, measure_node_info_t *measure_node_info, char *name, char *value);


    /**** exported functions ****/

char *scenario_load(char *filename)
{
    setting_t *root_setting = setting_create("scenario", NULL);
    setting_set_value(root_setting, filename);

    setting_t *current_setting = NULL;
    setting_t *parent_setting = root_setting;

    error_string[0] = '\0';

    rs_debug(DEBUG_SCENARIO, "loading scenario from file '%s'", filename);

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
        else if (isalnum(c) || c == '_' || c == '.' || c == '-') {
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
        all_ok = all_ok && load_post_process();
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
    rs_debug(DEBUG_SCENARIO, "saving scenario to file '%s'", filename);

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

    setting_destroy(root_setting);

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

bool load_post_process()
{
    uint16 i, j, node_count;
    node_t **node_list = rs_system_get_node_list_copy(&node_count);

    for (i = 0; i < node_count; i++) {
        node_t *node = node_list[i];

        for (j = 0; j < node->ip_info->route_count; j++) {
            ip_route_t *route = node->ip_info->route_list[j];

            if (route->further_info != NULL) { /* workaround */
                route->next_hop = rs_system_find_node_by_name((char *) route->further_info);
                free(route->further_info);
            }

            free(route->dst_bit_expanded);
            route->dst_bit_expanded = NULL;
        }

        if (node->measure_info->connect_dst_node != NULL) {
            char *node_name = (char *) node->measure_info->connect_dst_node;

            node->measure_info->connect_dst_node = rs_system_find_node_by_name(node_name);

            free(node_name);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    return TRUE;
}

bool parse_setting_tree(setting_t *setting, char *path, node_t *node)
{
    rs_assert(setting != NULL);

    if (setting->setting_count == 0 && setting->value != NULL) {
        if (setting->parent_setting == NULL) {
            return TRUE;
        }

        if (strcmp(setting->parent_setting->name, "system") == 0) {
            return apply_system_setting(path, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "display") == 0) {
            return apply_display_setting(path, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "events") == 0) {
            return apply_events_setting(path, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "phy") == 0) {
            return apply_phy_setting(path, node->phy_info, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "mobility") == 0) {
            return apply_mobility_setting(path, node->phy_info->mobility_list[node->phy_info->mobility_count - 1], setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "mac") == 0) {
            return apply_mac_setting(path, node->mac_info, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "ip") == 0) {
            return apply_ip_setting(path, node->ip_info, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "route") == 0) {
            return apply_route_setting(path, node->ip_info->route_list[node->ip_info->route_count - 1], setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "icmp") == 0) {
            return apply_icmp_setting(path, node->icmp_info, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "rpl") == 0) {
            return apply_rpl_setting(path, node->rpl_info, setting->name, setting->value);
        }
        else if (strcmp(setting->parent_setting->name, "measure") == 0) {
            return apply_measure_setting(path, node->measure_info, setting->name, setting->value);
        }
        else {
            sprintf(error_string, "unexpected setting '%s.%s'", path, setting->name);
            return FALSE;
        }
    }
    else {
        if (strcmp(setting->name, "node") == 0) {
            node = node_create();

            measure_node_init(node);
            phy_node_init(node, "", 0, 0);
            mac_node_init(node, "");
            ip_node_init(node, "");
            icmp_node_init(node);
            rpl_node_init(node);

            rs_system_add_node(node);
        }
        else if (strcmp(setting->name, "mobility") == 0) {
            phy_node_add_mobility(node, 0, 0, 0, 0);
        }
        else if (strcmp(setting->name, "route") == 0) {
            ip_node_add_route(node, "0", 0, NULL, IP_ROUTE_TYPE_MANUAL, NULL);
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

    setting = setting_create("auto_wake_nodes", system_setting);
    sprintf(text, "%s", rs_system->auto_wake_nodes ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("deterministic_random", system_setting);
    sprintf(text, "%s", rs_system->deterministic_random ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("simulation_second", system_setting);
    sprintf(text, "%d", rs_system->simulation_second);
    setting_set_value(setting, text);

    setting = setting_create("width", system_setting);
    sprintf(text, "%.02f", rs_system->width);
    setting_set_value(setting, text);

    setting = setting_create("height", system_setting);
    sprintf(text, "%.02f", rs_system->height);
    setting_set_value(setting, text);

    setting = setting_create("no_link_dist_thresh", system_setting);
    sprintf(text, "%.02f", rs_system->no_link_dist_thresh);
    setting_set_value(setting, text);

    setting = setting_create("no_link_quality_thresh", system_setting);
    sprintf(text, "%.02f", rs_system->no_link_quality_thresh);
    setting_set_value(setting, text);

    setting = setting_create("transmission_time", system_setting);
    sprintf(text, "%d", rs_system->transmission_time);
    setting_set_value(setting, text);

    setting = setting_create("mac_pdu_timeout", system_setting);
    sprintf(text, "%d", rs_system->mac_pdu_timeout);
    setting_set_value(setting, text);

    setting = setting_create("ip_pdu_timeout", system_setting);
    sprintf(text, "%d", rs_system->ip_pdu_timeout);
    setting_set_value(setting, text);

    setting = setting_create("ip_queue_size", system_setting);
    sprintf(text, "%d", rs_system->ip_queue_size);
    setting_set_value(setting, text);

    setting = setting_create("ip_neighbor_timeout", system_setting);
    sprintf(text, "%d", rs_system->ip_neighbor_timeout);
    setting_set_value(setting, text);

    setting = setting_create("measure_pdu_timeout", system_setting);
    sprintf(text, "%d", rs_system->measure_pdu_timeout);
    setting_set_value(setting, text);

    setting = setting_create("rpl_auto_sn_inc_interval", system_setting);
    sprintf(text, "%d", rs_system->rpl_auto_sn_inc_interval);
    setting_set_value(setting, text);

    setting = setting_create("rpl_dao_supported", system_setting);
    sprintf(text, "%s", rs_system->rpl_dao_supported ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_dao_trigger", system_setting);
    sprintf(text, "%s", rs_system->rpl_dao_trigger ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_dio_interval_doublings", system_setting);
    sprintf(text, "%d", rs_system->rpl_dio_interval_doublings);
    setting_set_value(setting, text);

    setting = setting_create("rpl_dio_interval_min", system_setting);
    sprintf(text, "%d", rs_system->rpl_dio_interval_min);
    setting_set_value(setting, text);

    setting = setting_create("rpl_dio_redundancy_constant", system_setting);
    sprintf(text, "%d", rs_system->rpl_dio_redundancy_constant);
    setting_set_value(setting, text);

    setting = setting_create("rpl_max_inc_rank", system_setting);
    sprintf(text, "%d", rs_system->rpl_max_inc_rank);
    setting_set_value(setting, text);

    setting = setting_create("rpl_poison_count", system_setting);
    sprintf(text, "%d", rs_system->rpl_poison_count);
    setting_set_value(setting, text);

    setting = setting_create("rpl_prefer_floating", system_setting);
    sprintf(text, "%s", rs_system->rpl_prefer_floating ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_start_silent", system_setting);
    sprintf(text, "%s", rs_system->rpl_start_silent ? "true" : "false");
    setting_set_value(setting, text);

    setting_t *display_setting = setting_create("display", system_setting);

    setting = setting_create("show_node_names", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_node_names ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_node_addresses", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_node_addresses ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_node_tx_power", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_node_tx_power ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_node_ranks", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_node_ranks ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_preferred_parent_arrows", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_preferred_parent_arrows ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_parent_arrows", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_parent_arrows ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("show_sibling_arrows", display_setting);
    sprintf(text, "%s", main_win_get_display_params()->show_sibling_arrows ? "true" : "false");
    setting_set_value(setting, text);

    setting_t *events_setting = setting_create("events", system_setting);

    setting = setting_create("sys_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(sys_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("sys_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(sys_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("sys_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(sys_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_neighbor_attach_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_neighbor_attach) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_neighbor_detach_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_neighbor_detach) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("phy_event_change_mobility_logging", events_setting);
    sprintf(text, "%s", event_get_logging(phy_event_change_mobility) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("mac_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(mac_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("mac_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(mac_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("mac_event_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(mac_event_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("mac_event_pdu_send_timeout_check_logging", events_setting);
    sprintf(text, "%s", event_get_logging(mac_event_pdu_send_timeout_check) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("mac_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(mac_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_pdu_send_timeout_check_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_pdu_send_timeout_check) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("ip_event_neighbor_cache_timeout_check_logging", events_setting);
    sprintf(text, "%s", event_get_logging(ip_event_neighbor_cache_timeout_check) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_ping_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_ping_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("icmp_event_ping_timeout_logging", events_setting);
    sprintf(text, "%s", event_get_logging(icmp_event_ping_timeout) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dis_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dis_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dis_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dis_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dio_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dio_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dio_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dio_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dao_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dao_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_dao_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_dao_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_neighbor_attach_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_neighbor_attach) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_neighbor_detach_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_neighbor_detach) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_forward_failure_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_forward_failure) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_forward_inconsistency_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_forward_inconsistency) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_trickle_t_timeout_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_trickle_t_timeout) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_trickle_i_timeout_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_trickle_i_timeout) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("rpl_event_seq_num_autoinc_logging", events_setting);
    sprintf(text, "%s", event_get_logging(rpl_event_seq_num_autoinc) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_node_wake_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_node_wake) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_node_kill_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_node_kill) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_pdu_send_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_pdu_send) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_pdu_receive_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_pdu_receive) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_update_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_update) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_hop_passed_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_hop_passed) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_hop_failed_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_hop_failed) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_hop_timeout_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_hop_timeout) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_established_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_established) ? "true" : "false");
    setting_set_value(setting, text);

    setting = setting_create("measure_event_connect_lost_logging", events_setting);
    sprintf(text, "%s", event_get_logging(measure_event_connect_lost) ? "true" : "false");
    setting_set_value(setting, text);

    uint16 i, j, node_count;
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

        setting = setting_create("battery_level", phy_setting);
        sprintf(text, "%.02f", node->phy_info->battery_level);
        setting_set_value(setting, text);

        setting = setting_create("tx_power", phy_setting);
        sprintf(text, "%.02f", node->phy_info->tx_power);
        setting_set_value(setting, text);

        setting = setting_create("mains_powered", phy_setting);
        sprintf(text, "%s", node->phy_info->mains_powered ? "true" : "false");
        setting_set_value(setting, text);

        for (j = 0; j < node->phy_info->mobility_count; j++) {
            phy_mobility_t *mobility = node->phy_info->mobility_list[j];

            setting_t *mobility_setting = setting_create("mobility", node_setting);

            setting = setting_create("trigger_time", mobility_setting);
            sprintf(text, "%d", mobility->trigger_time);
            setting_set_value(setting, text);

            setting = setting_create("duration", mobility_setting);
            sprintf(text, "%d", mobility->duration);
            setting_set_value(setting, text);

            setting = setting_create("dest_x", mobility_setting);
            sprintf(text, "%.02f", mobility->dest_x);
            setting_set_value(setting, text);

            setting = setting_create("dest_y", mobility_setting);
            sprintf(text, "%.02f", mobility->dest_y);
            setting_set_value(setting, text);
        }

        setting_t *mac_setting = setting_create("mac", node_setting);

        setting = setting_create("address", mac_setting);
        setting_set_value(setting, node->mac_info->address);

        setting_t *ip_setting = setting_create("ip", node_setting);

        setting = setting_create("address", ip_setting);
        setting_set_value(setting, node->ip_info->address);

        for (j = 0; j < node->ip_info->route_count; j++) {
            ip_route_t *route= node->ip_info->route_list[j];

            if (route->type != IP_ROUTE_TYPE_MANUAL) {
                continue;
            }

            setting_t *route_setting = setting_create("route", ip_setting);

            setting = setting_create("dst", route_setting);
            setting_set_value(setting, route->dst);

            setting = setting_create("prefix_len", route_setting);
            sprintf(text, "%d", route->prefix_len);
            setting_set_value(setting, text);

            setting = setting_create("next_hop", route_setting);
            setting_set_value(setting, route->next_hop->phy_info->name);
        }

        setting_t *icmp_setting = setting_create("icmp", node_setting);

        if (node->icmp_info->ping_ip_address != NULL) {
            setting = setting_create("ping_ip_address", icmp_setting);
            setting_set_value(setting, node->icmp_info->ping_ip_address);

            setting = setting_create("ping_interval", icmp_setting);
            sprintf(text, "%d", node->icmp_info->ping_interval);
            setting_set_value(setting, text);

            setting = setting_create("ping_timeout", icmp_setting);
            sprintf(text, "%d", node->icmp_info->ping_timeout);
            setting_set_value(setting, text);
        }

        setting_t *rpl_setting = setting_create("rpl", node_setting);

        setting = setting_create("storing", rpl_setting);
        sprintf(text, "%s", node->rpl_info->storing ? "true" : "false");
        setting_set_value(setting, text);

        setting = setting_create("dao_supported", rpl_setting);
        sprintf(text, "%s", node->rpl_info->root_info->dao_supported ? "true" : "false");
        setting_set_value(setting, text);

        setting = setting_create("dao_trigger", rpl_setting);
        sprintf(text, "%s", node->rpl_info->root_info->dao_trigger ? "true" : "false");
        setting_set_value(setting, text);

        setting = setting_create("dio_interval_doublings", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->dio_interval_doublings);
        setting_set_value(setting, text);

        setting = setting_create("dio_interval_min", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->dio_interval_min);
        setting_set_value(setting, text);

        setting = setting_create("dio_redundancy_constant", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->dio_redundancy_constant);
        setting_set_value(setting, text);

        if (node->rpl_info->root_info->configured_dodag_id != NULL) {
            setting = setting_create("configured_dodag_id", rpl_setting);
            setting_set_value(setting, node->rpl_info->root_info->configured_dodag_id);
        }

        setting = setting_create("dodag_pref", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->dodag_pref);
        setting_set_value(setting, text);

        setting = setting_create("grounded", rpl_setting);
        sprintf(text, "%s", node->rpl_info->root_info->grounded ? "true" : "false");
        setting_set_value(setting, text);

        setting = setting_create("max_rank_inc", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->max_rank_inc);
        setting_set_value(setting, text);

        setting = setting_create("min_hop_rank_inc", rpl_setting);
        sprintf(text, "%d", node->rpl_info->root_info->min_hop_rank_inc);
        setting_set_value(setting, text);

        setting_t *measure_setting = setting_create("measure", node_setting);

        if (node->measure_info->connect_dst_node != NULL) {
            setting = setting_create("connect_dst_node", measure_setting);
            sprintf(text, "%s", node->measure_info->connect_dst_node->phy_info->name);
            setting_set_value(setting, text);
        }
    }

    if (node_list != NULL) {
        free(node_list);
    }

    return root_setting;
}

bool apply_system_setting(char *path, char *name, char *value)
{
    if (strcmp(name, "auto_wake_nodes") == 0) {
        rs_system->auto_wake_nodes = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "deterministic_random") == 0) {
        rs_system->deterministic_random = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "simulation_second") == 0) {
        rs_system->simulation_second = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "width") == 0) {
        rs_system->width = strtof(value, NULL);
    }
    else if (strcmp(name, "height") == 0) {
        rs_system->height = strtof(value, NULL);
    }
    else if (strcmp(name, "no_link_dist_thresh") == 0) {
        rs_system->no_link_dist_thresh = strtof(value, NULL);
    }
    else if (strcmp(name, "no_link_quality_thresh") == 0) {
        rs_system->no_link_quality_thresh = strtof(value, NULL);
    }
    else if (strcmp(name, "transmission_time") == 0) {
        rs_system->transmission_time = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "mac_pdu_timeout") == 0) {
        rs_system->mac_pdu_timeout = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "ip_pdu_timeout") == 0) {
        rs_system->ip_pdu_timeout = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "ip_queue_size") == 0) {
        rs_system->ip_queue_size = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "ip_neighbor_timeout") == 0) {
        rs_system->ip_neighbor_timeout = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "measure_pdu_timeout") == 0) {
        rs_system->measure_pdu_timeout = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_auto_sn_inc_interval") == 0) {
        rs_system->rpl_auto_sn_inc_interval = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_dao_supported") == 0) {
        rs_system->rpl_dao_supported = (strcmp(value, "true"));
    }
    else if (strcmp(name, "rpl_dao_trigger") == 0) {
        rs_system->rpl_dao_trigger = (strcmp(value, "true"));
    }
    else if (strcmp(name, "rpl_dio_interval_doublings") == 0) {
        rs_system->rpl_dio_interval_doublings = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_dio_interval_min") == 0) {
        rs_system->rpl_dio_interval_min = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_dio_redundancy_constant") == 0) {
        rs_system->rpl_dio_redundancy_constant = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_max_inc_rank") == 0) {
        rs_system->rpl_max_inc_rank = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_poison_count") == 0) {
        rs_system->rpl_poison_count = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "rpl_prefer_floating") == 0) {
        rs_system->rpl_prefer_floating = (strcmp(value, "true"));
    }
    else if (strcmp(name, "rpl_start_silent") == 0) {
        rs_system->rpl_start_silent = (strcmp(value, "true"));
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_display_setting(char *path, char *name, char *value)
{
    if (strcmp(name, "show_node_addresses") == 0) {
        main_win_get_display_params()->show_node_addresses = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_node_names") == 0) {
        main_win_get_display_params()->show_node_names = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_node_ranks") == 0) {
        main_win_get_display_params()->show_node_ranks = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_node_tx_power") == 0) {
        main_win_get_display_params()->show_node_tx_power = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_parent_arrows") == 0) {
        main_win_get_display_params()->show_parent_arrows = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_preferred_parent_arrows") == 0) {
        main_win_get_display_params()->show_preferred_parent_arrows = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "show_sibling_arrows") == 0) {
        main_win_get_display_params()->show_sibling_arrows = (strcmp(value, "true") == 0);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_events_setting(char *path, char *name, char *value)
{
    if (strcmp(name, "sys_event_node_wake_logging") == 0) {
        event_set_logging(sys_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "sys_event_node_kill_logging") == 0) {
        event_set_logging(sys_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "sys_event_pdu_receive_logging") == 0) {
        event_set_logging(sys_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_node_wake_logging") == 0) {
        event_set_logging(phy_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_node_kill_logging") == 0) {
        event_set_logging(phy_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_pdu_send_logging") == 0) {
        event_set_logging(phy_event_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_pdu_receive_logging") == 0) {
        event_set_logging(phy_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_neighbor_attach_logging") == 0) {
        event_set_logging(phy_event_neighbor_attach, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_neighbor_detach_logging") == 0) {
        event_set_logging(phy_event_neighbor_detach, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "phy_event_change_mobility_logging") == 0) {
        event_set_logging(phy_event_change_mobility, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "mac_event_node_wake_logging") == 0) {
        event_set_logging(mac_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "mac_event_node_kill_logging") == 0) {
        event_set_logging(mac_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "mac_event_pdu_send_logging") == 0) {
        event_set_logging(mac_event_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "mac_event_pdu_send_timeout_check_logging") == 0) {
        event_set_logging(mac_event_pdu_send_timeout_check, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "mac_event_pdu_receive_logging") == 0) {
        event_set_logging(mac_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_node_wake_logging") == 0) {
        event_set_logging(ip_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_node_kill_logging") == 0) {
        event_set_logging(ip_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_pdu_send_logging") == 0) {
        event_set_logging(ip_event_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_pdu_send_timeout_check_logging") == 0) {
        event_set_logging(ip_event_pdu_send_timeout_check, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_pdu_receive_logging") == 0) {
        event_set_logging(ip_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "ip_event_neighbor_cache_timeout_check_logging") == 0) {
        event_set_logging(ip_event_neighbor_cache_timeout_check, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_node_wake_logging") == 0) {
        event_set_logging(icmp_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_node_kill_logging") == 0) {
        event_set_logging(icmp_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_pdu_send_logging") == 0) {
        event_set_logging(icmp_event_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_pdu_receive_logging") == 0) {
        event_set_logging(icmp_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_ping_send_logging") == 0) {
        event_set_logging(icmp_event_ping_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "icmp_event_ping_timeout_logging") == 0) {
        event_set_logging(icmp_event_ping_timeout, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_node_wake_logging") == 0) {
        event_set_logging(rpl_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_node_kill_logging") == 0) {
        event_set_logging(rpl_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dis_pdu_send_logging") == 0) {
        event_set_logging(rpl_event_dis_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dis_pdu_receive_logging") == 0) {
        event_set_logging(rpl_event_dis_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dio_pdu_send_logging") == 0) {
        event_set_logging(rpl_event_dio_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dio_pdu_receive_logging") == 0) {
        event_set_logging(rpl_event_dio_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dao_pdu_send_logging") == 0) {
        event_set_logging(rpl_event_dao_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_dao_pdu_receive_logging") == 0) {
        event_set_logging(rpl_event_dao_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_neighbor_attach_logging") == 0) {
        event_set_logging(rpl_event_neighbor_attach, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_neighbor_detach_logging") == 0) {
        event_set_logging(rpl_event_neighbor_detach, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_forward_failure_logging") == 0) {
        event_set_logging(rpl_event_forward_failure, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_forward_inconsistency_logging") == 0) {
        event_set_logging(rpl_event_forward_inconsistency, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_trickle_t_timeout_logging") == 0) {
        event_set_logging(rpl_event_trickle_t_timeout, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_trickle_i_timeout_logging") == 0) {
        event_set_logging(rpl_event_trickle_i_timeout, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "rpl_event_seq_num_autoinc_logging") == 0) {
        event_set_logging(rpl_event_seq_num_autoinc, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_node_wake_logging") == 0) {
        event_set_logging(measure_event_node_wake, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_node_kill_logging") == 0) {
        event_set_logging(measure_event_node_kill, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_pdu_send_logging") == 0) {
        event_set_logging(measure_event_pdu_send, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_pdu_receive_logging") == 0) {
        event_set_logging(measure_event_pdu_receive, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_update_logging") == 0) {
        event_set_logging(measure_event_connect_update, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_hop_passed_logging") == 0) {
        event_set_logging(measure_event_connect_hop_passed, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_hop_failed_logging") == 0) {
        event_set_logging(measure_event_connect_hop_failed, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_hop_timeout_logging") == 0) {
        event_set_logging(measure_event_connect_hop_timeout, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_established_logging") == 0) {
        event_set_logging(measure_event_connect_established, (strcmp(value, "true") == 0));
    }
    else if (strcmp(name, "measure_event_connect_lost_logging") == 0) {
        event_set_logging(measure_event_connect_lost, (strcmp(value, "true") == 0));
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
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
    else if (strcmp(name, "battery_level") == 0) {
        phy_node_info->battery_level = strtof(value, NULL);
    }
    else if (strcmp(name, "tx_power") == 0) {
        phy_node_info->tx_power = strtof(value, NULL);
    }
    else if (strcmp(name, "mains_powered") == 0) {
        phy_node_info->mains_powered = (strcmp(value, "true") == 0);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_mobility_setting(char *path, phy_mobility_t *mobility, char *name, char *value)
{
    if (strcmp(name, "trigger_time") == 0) {
        mobility->trigger_time = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "duration") == 0) {
        mobility->duration = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "dest_x") == 0) {
        mobility->dest_x = strtof(value, NULL);
    }
    else if (strcmp(name, "dest_y") == 0) {
        mobility->dest_y = strtof(value, NULL);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_mac_setting(char *path, mac_node_info_t *mac_node_info, char *name, char *value)
{
    if (strcmp(name, "address") == 0) {
        mac_node_info->address = strdup(value);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_ip_setting(char *path, ip_node_info_t *ip_node_info, char *name, char *value)
{
    if (strcmp(name, "address") == 0) {
        ip_node_info->address = strdup(value);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_route_setting(char *path, ip_route_t *route, char *name, char *value)
{
    if (strcmp(name, "dst") == 0) {
        route->dst = strdup(value);
    }
    else if (strcmp(name, "prefix_len") == 0) {
        route->prefix_len = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "next_hop") == 0) {
        route->further_info = strdup(value); /* workaround */
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_icmp_setting(char *path, icmp_node_info_t *icmp_node_info, char *name, char *value)
{
    if (strcmp(name, "ping_ip_address") == 0) {
        icmp_node_info->ping_ip_address = strdup(value);
    }
    else if (strcmp(name, "ping_interval") == 0) {
        icmp_node_info->ping_interval= strtol(value, NULL, 10);
    }
    else if (strcmp(name, "ping_timeout") == 0) {
        icmp_node_info->ping_timeout= strtol(value, NULL, 10);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_rpl_setting(char *path, rpl_node_info_t *rpl_node_info, char *name, char *value)
{
    if (strcmp(name, "storing") == 0) {
        rpl_node_info->storing = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "dao_supported") == 0) {
        rpl_node_info->root_info->dao_supported = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "dao_trigger") == 0) {
        rpl_node_info->root_info->dao_trigger = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "dio_interval_doublings") == 0) {
        rpl_node_info->root_info->dio_interval_doublings = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "dio_interval_min") == 0) {
        rpl_node_info->root_info->dio_interval_min = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "dio_redundancy_constant") == 0) {
        rpl_node_info->root_info->dio_redundancy_constant = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "configured_dodag_id") == 0) {
        rpl_node_info->root_info->configured_dodag_id = strdup(value);
    }
    else if (strcmp(name, "dodag_pref") == 0) {
        rpl_node_info->root_info->dodag_pref = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "grounded") == 0) {
        rpl_node_info->root_info->grounded = (strcmp(value, "true") == 0);
    }
    else if (strcmp(name, "max_rank_inc") == 0) {
        rpl_node_info->root_info->max_rank_inc = strtol(value, NULL, 10);
    }
    else if (strcmp(name, "min_hop_rank_inc") == 0) {
        rpl_node_info->root_info->min_hop_rank_inc = strtol(value, NULL, 10);
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}

bool apply_measure_setting(char *path, measure_node_info_t *measure_node_info, char *name, char *value)
{
    if (strcmp(name, "connect_dst_node") == 0) {
        measure_node_info->connect_dst_node = (node_t *) strdup(value); /* workaround */
    }
    else {
        sprintf(error_string, "unexpected setting '%s.%s'", path, name);
        return FALSE;
    }

    return TRUE;
}
