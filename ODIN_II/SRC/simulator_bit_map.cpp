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

inline static bit_tree_map *bit_map_node()
{
	bit_tree_map *node = (bit_tree_map *)vtr::malloc(sizeof(bit_tree_map));
	for(int i=0; i<3; i++)
		node->branch[i] = NULL;
	
	return node;
}

inline static bit_tree_root *bit_map_root(const char *related_node_name, int depth)
{
	bit_tree_root *root = (bit_tree_root *)vtr::malloc(sizeof(bit_tree_root));
	root->depth = depth;
	root->related_node_name = vtr::strdup(related_node_name);
	root->branch[_0] = bit_map_node();
	root->branch[_1] = bit_map_node();
	return root;
}

inline static bit_tree_map *free_bit_branches(bit_tree_map *bit_map)
{
	if(bit_map)
	{
		if(!bit_map->is_leaf)
		{
			bit_map->branch[_0] = free_bit_branches(bit_map->branch[_0]);
			bit_map->branch[_1] = free_bit_branches(bit_map->branch[_1]);
			bit_map->branch[_x] = free_bit_branches(bit_map->branch[_x]);
		}
		vtr::free(bit_map);
	}
	return NULL;
}

bit_tree_root *free_bit_tree(bit_tree_root *bit_map)
{
	if(bit_map)
	{
		vtr::free(bit_map->related_node_name);
		bit_map->branch[_0] = free_bit_branches(bit_map->branch[_0]);
		bit_map->branch[_1] = free_bit_branches(bit_map->branch[_1]);
		vtr::free(bit_map);
	}
	return NULL;
}


bit_tree_root *consume_bit_map_line(std::vector<std::string> lines, std::vector<char> results, const char *related_node_name)
{
	if(lines.empty() || results.empty())
		return NULL;

	bit_tree_root *root = bit_map_root(related_node_name, lines[0].size());

	for(int i=0; i < lines.size(); i++)
	{
		bit_map_values cur_trunk = get_mapped_value(results[i]);
		bit_tree_map *cur_node = root->branch[cur_trunk];

		if(lines[i].size() != lines[0].size())
			error_message(SIMULATION_ERROR, -1, -1, "mismatched length in input lut @ line %d for node <%s>",i,root->related_node_name);

		for(int j=0; j < lines[i].size(); j++)
		{
			bit_map_values cur_value = get_mapped_value(lines[i][j]);

			if(!cur_node->branch[cur_value])
				cur_node->branch[cur_value] = bit_map_node();

			if(j+1 == lines[i].size())
				cur_node->is_leaf = true;
			
		}
	}
	return root;
}

static signed char explore_trunk(bit_tree_map *trunk, char result, const std::string& line, char set_to)
{
	int end = line.size()-1;
	bool error = false;
	std::vector<bit_tree_map*> Q = {trunk};
	std::vector<bit_tree_map*> new_Q;
	for(int j=0; j <= end && !Q.empty() && !error; j++)
	{
		bit_map_values cur_value = get_mapped_value(line[j]);
		for(int i=0; i < Q.size() && !error; i++)
		{
			bit_tree_map *bit_map = Q[i];
			if(bit_map->is_leaf)
			{
				if(result)
				{
					error = true;
					result = '!';
				}
				else
				{
					result = set_to;
				}
			}
			else
			{
				if(bit_map->branch[cur_value])
					new_Q.push_back(bit_map->branch[cur_value]);

				//include unknown in defined search
				if(cur_value != _x && bit_map->branch[_x])
					new_Q.push_back(bit_map->branch[_x]);
			}
		}

		Q.clear();
		Q = new_Q;
		new_Q.clear();
	}
	return result;
}

char find_result(bit_tree_root *root, std::string line)
{
	if(!root)
		error_message(SIMULATION_ERROR, -1, -1, "root bit_map is empty");

	if(line == "")
		return 3;
 
	char result = 0;


	if(root->depth > line.size())
		error_message(SIMULATION_ERROR, -1, -1, "input missmatch: bit map length > input length ");
	else if(root->depth < line.size())
		error_message(SIMULATION_ERROR, -1, -1, "input missmatch: bit map length < input length ");

	result = explore_trunk(root->branch[_0], result, line, '0');
	result = explore_trunk(root->branch[_1], result, line, '1');
	if(result == '!')
	{
		warning_message(SIMULATION_ERROR, -1, -1, "ambiguous bit map for .name <%s> with multiple, or no result for pattern", root->related_node_name);
		result = 'x';
	}
	else if(!result)
	{
		result = 'x';
	}

	return result;
}