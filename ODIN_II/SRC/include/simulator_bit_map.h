#ifndef SIMULATOR_BIT_MAP_H
#define SIMULATOR_BIT_MAP_H

#include <string>
#include <vector>

typedef struct bit_tree_map_t bit_tree_map;
struct bit_tree_map_t
{
	bit_tree_map *branch[3];
    bool is_leaf;
};

typedef struct bit_tree_root_t
{
	bit_tree_map *branch[2];
    int depth;
    char *related_node_name;
}bit_tree_root;

typedef enum
{
    _0 = 0,
    _1 = 1,
    _x = 2
}bit_map_values;

bit_map_values get_mapped_value(signed char value);

bit_tree_root *free_bit_tree(bit_tree_root *bit_map);
bit_tree_root *consume_bit_map_line(std::vector<std::string> lines, std::vector<char> results, const char *related_node_name);
char find_result(bit_tree_root *bit_map, std::string line);

#endif