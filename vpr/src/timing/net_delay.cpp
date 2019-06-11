#include <cstdio>
using namespace std;

#include "vtr_memory.h"
#include "vtr_log.h"

#include "vpr_types.h"
#include "vpr_error.h"

#include "globals.h"
#include "net_delay.h"
#include "route_tree_timing.h"


 /********************** Variables local to this module ***********************/
 
 /* Array below allows mapping from any rr_node to any rt_node currently in
  *   * the rt_tree.                                                              */
 
static std::unordered_map<int,float> inode_to_Tdel_map;


/*********************** Subroutines local to this module ********************/

static void load_one_net_delay(vtr::vector<ClusterNetId, float *> &net_delay, ClusterNetId net_id);

static void load_one_net_delay_recurr(t_rt_node* node, ClusterNetId net_id);

static void load_one_constant_net_delay(vtr::vector<ClusterNetId, float *> &net_delay, ClusterNetId net_id, float delay_value);

/*************************** Subroutine definitions **************************/

/* Allocates space for the net_delay data structure   *
* [0..nets.size()-1][1..num_pins-1]. I chunk the data *
* to save space on large problems.                    */
vtr::vector<ClusterNetId, float *> alloc_net_delay(vtr::t_chunk *chunk_list_ptr){
	auto& cluster_ctx = g_vpr_ctx.clustering();
	vtr::vector<ClusterNetId, float *> net_delay; /* [0..nets.size()-1][1..num_pins-1] */

    auto nets = cluster_ctx.clb_nlist.nets();
	net_delay.resize(nets.size());

	for (auto net_id : nets) {
		float* tmp_ptr = (float *) vtr::chunk_malloc(cluster_ctx.clb_nlist.net_sinks(net_id).size() * sizeof(float), chunk_list_ptr);

		net_delay[net_id] = tmp_ptr - 1; /* [1..num_pins-1] */

        //Ensure the net delays are initialized with non-garbage values
		for (size_t ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ++ipin) {
            net_delay[net_id][ipin] = std::numeric_limits<float>::quiet_NaN();
        }
	}

	return (net_delay);
}

void free_net_delay(vtr::vector<ClusterNetId, float *> &net_delay,
		vtr::t_chunk *chunk_list_ptr){


	net_delay.clear();
    vtr::free_chunk_memory(chunk_list_ptr);
}

void load_net_delay_from_routing(vtr::vector<ClusterNetId, float *> &net_delay) {

	/* This routine loads net_delay[0..nets.size()-1][1..num_pins-1].  Each entry   *
	 * is the Elmore delay from the net source to the appropriate sink.  Both    *
	 * the rr_graph and the routing traceback must be completely constructed     *
	 * before this routine is called, and the net_delay array must have been     *
	 * allocated.                                                                */
	auto& cluster_ctx = g_vpr_ctx.clustering();


	for (auto net_id : cluster_ctx.clb_nlist.nets()) {
		if (cluster_ctx.clb_nlist.net_is_ignored(net_id)) {
			load_one_constant_net_delay(net_delay, net_id, 0.);
		} else {
			load_one_net_delay(net_delay, net_id); 
		}
	}
    inode_to_Tdel_map.clear();
	free_route_tree_timing_structs(); 
}


static void load_one_net_delay(vtr::vector<ClusterNetId, float *> &net_delay, ClusterNetId net_id) {
	
	/* This routine loads delay values of one net in the 2-dimensional vector    *
     * net_delay[net_id][1..num_pins-1]. First, it constructs the route tree     *
     * from the traceback, next it updates the values for R, C, and Tdel.        *
     * Next, it walks the route tree recursively, updating the net delay         *
     * array during the traversal. Finally it frees teh route tree.              *
	 * allocated.                                                                */

	t_rt_node *rt_root;
    
    auto& cluster_ctx = g_vpr_ctx.clustering();
    auto& route_ctx = g_vpr_ctx.routing();


    int inode;

    rt_root = traceback_to_route_tree(net_id); //obtain the root of the tree from the traceback
    rt_root->parent_switch = OPEN;
    load_new_subtree_R_upstream(rt_root); //load in the resistance values for the RT Tree
    load_new_subtree_C_downstream(rt_root); //load in the capacitance values for the RT Tree
    load_route_tree_Tdel(rt_root, 0.); //load the time delay values for the RT Tree
    // now I need to traverse the tree to fill in the values for the net_delay array.
	load_one_net_delay_recurr(rt_root, net_id);


	for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++)
    {
        inode = route_ctx.net_rr_terminals[net_id][ipin];
        net_delay[net_id][ipin] = inode_to_Tdel_map.find(inode)->second;
    }
    free_route_tree(rt_root); // free the route tree
}

static void load_one_net_delay_recurr(t_rt_node* node, ClusterNetId net_id){
	/* This routine recursively traverses the route tree. It first searches      *
     * for the indices of net_delay which correspond to the input's inode.       *
     * Once it is found, the net_delay is immediately updated. Then it needs     *
     * to process all of the children.                                           */

	inode_to_Tdel_map[node->inode] = node->Tdel; // add to the map

    // finished processing the node, process the children.
    
    for (t_linked_rt_edge* edge = node->u.child_list; edge != nullptr; edge = edge->next) {
        load_one_net_delay_recurr(edge->child, net_id);
    }
}

static void load_one_constant_net_delay(vtr::vector<ClusterNetId, float *> &net_delay, ClusterNetId net_id, float delay_value) {

	/* Sets each entry of the net_delay array for net inet to delay_value.     */
	auto& cluster_ctx = g_vpr_ctx.clustering();

	for (unsigned int ipin = 1; ipin < cluster_ctx.clb_nlist.net_pins(net_id).size(); ipin++)
		net_delay[net_id][ipin] = delay_value;
}

