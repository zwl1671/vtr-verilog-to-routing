#ifndef SIMULATOR_BIT_MAP_H
#define SIMULATOR_BIT_MAP_H

#include <string>
#include <vector>

typedef struct bit_tree_map_t bit_tree_map;
struct bit_tree_map_t
{
	bit_tree_map *childs[3];
	char my_value;
	int depth;
    int result_len;
	char *result;
    char *related_node_name;
    bool is_root;
    bool is_leaf;
};

typedef enum
{
    _0 = 0,
    _1 = 1,
    _x = 2
}bit_map_values;

bit_map_values get_mapped_value(signed char value);

bit_tree_map *free_bit_tree(bit_tree_map *bit_map);
bit_tree_map *consume_bit_map_line(std::vector<std::string> lines, std::vector<std::string> results, const char *related_node_name);
std::string find_result(bit_tree_map *bit_map, std::string line);

#endif