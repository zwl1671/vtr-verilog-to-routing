#include "simulator_bit_map.h"
#include "vtr_memory.h"
#include "vtr_util.h"
#include "odin_util.h"
#include <vector>

bit_map_values get_mapped_value(signed char value)
{
	if(value == 0 || value == '0')
		return _0;
	else if(value == 1 || value == '1')
		return _1;
	else
		return _x;
}

inline static bit_tree_map *bit_map_node(const char *related_node_name, char value, int depth, int result_len)
{
	bit_tree_map *node = (bit_tree_map *)vtr::malloc(sizeof(bit_tree_map));
	for(int i=0; i<3; i++)
	{
		node->childs[i] = NULL;
	}
	node->my_value = value;
	node->depth = depth;
	node->related_node_name = vtr::strdup(related_node_name);
	node->result_len = result_len;
	return node;
}

bit_tree_map *free_bit_tree(bit_tree_map *bit_map)
{
	if(!bit_map)
		return NULL;

	if(bit_map->is_leaf)
	{
		vtr::free(bit_map->related_node_name);
		vtr::free(bit_map);
		return NULL;
	}
	else
	{
		bit_map->childs[_0] = free_bit_tree(bit_map->childs[_0]);
		bit_map->childs[_1] = free_bit_tree(bit_map->childs[_1]);
		bit_map->childs[_x] = free_bit_tree(bit_map->childs[_x]);
		vtr::free(bit_map);
		return NULL;
	}
}

bit_tree_map *consume_bit_map_line(std::vector<std::string> lines, std::vector<std::string> results, const char *related_node_name)
{
	if(lines.empty() || results.empty())
		return NULL;

	bit_tree_map *root = bit_map_node(related_node_name, 0, lines[0].size(), results[0].size());
	root->is_root = true;

	for(int i=0; i < lines.size(); i++)
	{
		bit_tree_map *cur_node = root;
		if(lines[i].size() != lines[0].size())
			error_message(SIMULATION_ERROR, -1, -1, "mismatched length in input lut @ line %d for node <%s>",i,root->related_node_name);

		if(results[i].size() != results[0].size())
			error_message(SIMULATION_ERROR, -1, -1, "mismatched length in input lut @ line %d for node <%s>",i,root->related_node_name);

		for(int j=1; j <= lines[i].size(); j++)
		{
			char input_value = lines[i][j-1];
			bit_map_values cur_value = get_mapped_value(input_value);

			if(!cur_node->childs[cur_value])
				cur_node->childs[cur_value] = bit_map_node(root->related_node_name, input_value, lines[i].size(), results[i].size());

			/* this holds the result */
			if(j == lines[i].size())
			{
				cur_node->result = vtr::strdup(results[i].c_str());
				cur_node->is_leaf = true;
			}
		}
	}
	return root;
}

std::string find_result(bit_tree_map *root, std::string line)
{
	if(!root)
		error_message(SIMULATION_ERROR, -1, -1, "root bit_map is empty");

	if(line == "")
		return "";

	std::vector<bit_tree_map*> Q = {root};
	std::vector<bit_tree_map*> new_Q;

	std::string result = "";

	for(int j=0; j < line.size(); j++)
	{
		if(Q.empty())
			break;

		char input_value = line[j];
		bit_map_values cur_value = get_mapped_value(input_value);

		for(int i=0; i < Q.size(); i++)
		{
			bit_tree_map *bit_map = Q[i];

			if(!bit_map->is_leaf && j == line.size()-1)
				error_message(SIMULATION_ERROR, -1, -1, "input missmatch: bit map length != input length ");

			if(!bit_map->is_root)
				printf("######## exploring node #%d @%d for node:%s with input: %s @input id:%d\n", i, bit_map->depth, bit_map->related_node_name, line.c_str(), j);
			
			/* this node holds the result */
			if(bit_map->is_leaf)
			{
				if(result == "")
					result = bit_map->result;
				else
				{
					warning_message(SIMULATION_ERROR, -1, -1, "ambiguous bit map for .name <%s> with multiple result for pattern", bit_map->related_node_name);
					break;
				}
			}
			else
			{
				if(bit_map->childs[cur_value])
					new_Q.push_back(bit_map->childs[cur_value]);

				//include unknown in defined search
				if(cur_value != _x && bit_map->childs[_x])
					new_Q.push_back(bit_map->childs[_x]);

			}
		}

		Q.clear();
		Q = new_Q;
		new_Q.clear();

	}

	if(result == "")
	{
		warning_message(SIMULATION_ERROR, -1, -1, "missmatch between input and lut depth");
		result = "x";
	}
	return result;
}