#!/usr/bin/python

# Copyright © 2013      University of Wisconsin-La Crosse.
#                         All rights reserved.
#
# See COPYING in top-level directory.
#
# $HEADER$
#
# You will need the following libraries to use this sample script.
# NetworkX
#   http://networkx.github.io/
# Matplotlib
#   http://matplotlib.org/
# PyGraphviz
#   http://networkx.lanl.gov/pygraphviz/
# Graphviz
#   http://www.graphviz.org/
#
#

import sys, re
import networkx as nx
import matplotlib.pyplot as plt

try:
    from networkx import graphviz_layout
except ImportError:
    raise ImportError("This example needs Graphviz and either PyGraphviz or Pydot")

#
# Check Command line arguments
#
if len(sys.argv) < 2:
    print "Error: Must supply a GraphML input file"
    sys.exit(-1)

filename = sys.argv[1]
output_filename = ""

if re.search(r'graphml', filename):
    output_filename = filename.strip("graphml")
    G = nx.read_graphml(filename)
elif re.search(r'gexf', filename):
    output_filename = filename.strip("gexf")
    G = nx.read_gexf(filename)
else:
    print "Error: Unknown file extension ", filename
    sys.exit(-1)

output_filename = output_filename + "png"

#
# Positions for all nodes
#
pos = nx.spring_layout(G)
pos = nx.random_layout(G)
pos = nx.graphviz_layout(G,prog='dot')
pos = nx.graphviz_layout(G,prog='twopi')

#print G.graph

#
# Define how the nodes should be drawn
#
sw_nodes = []
other_nodes = []
for (u,d) in G.nodes(data=True):
    if 'type' in d and unicode(d['type']) == "switch":
        sw_nodes.append(u)
    else:
        other_nodes.append(u)

nx.draw_networkx_nodes(G, pos, nodelist=sw_nodes,    node_size=500,node_color='r')
nx.draw_networkx_nodes(G, pos, nodelist=other_nodes, node_size=200,node_color='b')

#
# Define how the edges should be drawn
#
sw_edges = []
other_edges = []
for (u,v,d) in G.edges(data=True):
    if 'source_type' in d and \
        'target_type' in d and \
        unicode(d['source_type']) == "switch" and \
        unicode(d['target_type']) == "switch":
        sw_edges.append((u,v))
    else:
        other_edges.append((u,v))

nx.draw_networkx_edges(G, pos, edgelist=sw_edges,    width=3,edge_color='r',style='dashed')
nx.draw_networkx_edges(G, pos, edgelist=other_edges, width=3,edge_color='b')

#
# Labels
#
#nx.draw_networkx_labels(G, pos, font_size=10,font_family='sans-serif',font_color='black')

# Display
plt.axis('off')
plt.savefig(output_filename)
#plt.show()

sys.exit()
