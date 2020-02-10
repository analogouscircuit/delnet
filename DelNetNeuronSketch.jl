module DelNetNeuronSketch

using Plots

# -------------------- Data Structures --------------------

# mutable struct NodeState
# 
# end

mutable struct Neuron
	v::Float64
	u::Float64
end

mutable struct Node
	idx_out_to_in::Int64
	num_in::Int64
	nodes_in::Array{Int64,1} 	# just for reference/construction
	idx_in_to_out::Int64
	num_out::Int64
	nodes_out::Array{Int64,1} 	#just for reference/construction
end

Node() = Node(0,0,Int64[],0,0,Int64[])


mutable struct Delay
	offset::Int64
	startidx::Int64
	len::Int64
	source::Int64
	target::Int64
end

mutable struct DelayNetwork
	inputs::Array{Float64, 1}
	outputs::Array{Float64, 1}
	delaybuf::Array{Float64, 1}
	invidx::Array{Int64, 1}
	nodes::Array{Node, 1}
	delays::Array{Delay, 1}
end

# function advance(dn::DelayNetwork) end
# function getinput(idx, dn::DelayNetwork) end
# function pushoutput(idx, dn::DelayNetwork) end

# -------------------- Functions --------------------
"""
"""
function advance(input, output, inverseidces, delays, delbuf)	
	#load input
	for i ∈ 1:length(input)
		buf_idx = delays[i].startidx + delays[i].offset
		delbuf[buf_idx] = input[i]	
	end

	# advance buffer
	for i ∈ 1:length(input)
		delays[i].offset = (delays[i].offset + 1) % delays[i].len
	end
	
	#pull output
	for i ∈ 1:length(input)
		output[ inverseidces[i] ] = delbuf[ delays[i].startidx + delays[i].offset ]	
	end
	output
end


"""
"""
function orderbuf(delay, delbuf) 
	[ delbuf[delay.startidx + ((delay.offset + k) % delay.len) ]
	 for k ∈ 0:delay.len-1] |> v -> map(x -> x == 0.0 ? "-" : "$(Int(round(x)))", v) |> reverse |> prod
end


"""
"""
function buftostr(buffer)
	buffer |> v -> map(x -> x == 0.0 ? "-" : "$(Int(round(x)))", v) |> prod
end


"""
"""
function blobgraph(n, p, delays::Array{Int, 1})
	@assert 0 ∉ delays "1 corresponds to no delay -- can't have 0 in delays"
	delmat = rand(n,n) |> m -> map(x -> x < p ? rand(delays) : 0, m)
	for k ∈ 1:n delmat[k,k] = 0 end
	return delmat
end


"""
"""
function delnetfromgraph(graph)
	n = size(graph)[1]
	deltot = sum(graph) 	# total number of delays to allocate
	numlines = sum( map(x -> x != 0 ? 1 : 0, graph) )
	inputs = zeros(numlines)
	outputs = zeros(numlines)
	delbuf = zeros(deltot)

	nodes = [Node() for _ ∈ 1:n] 
	delays = Array{Delay, 1}(undef,numlines)


	delcount = 1
	startidx = 1
	total = 0
	for i ∈ 1:n
		for j ∈ 1:n
			if graph[i,j] != 0
				total += graph[i,j]
				push!(nodes[i].nodes_out, j)
				nodes[i].num_out += 1
				push!(nodes[j].nodes_in, i)
				delays[delcount] = Delay(0, startidx, graph[i,j], i, j)
				startidx += graph[i,j]
				delcount += 1
			end
		end
	end

	num_outputs    = [ nd.num_out for nd ∈ nodes ]
	in_base_idcs = [ sum(num_outputs[1:k]) for k ∈ 1:n-1 ]
	in_base_idcs = [1; in_base_idcs[1:end] .+ 1]

	idx = 1
	for i ∈ 1:length(nodes)
		nodes[i].num_in = length(nodes[i].nodes_in)
		nodes[i].idx_out_to_in = idx
		idx += nodes[i].num_in
		nodes[i].idx_in_to_out = in_base_idcs[i]
	end

	num_inputs    = [ nd.num_in for nd ∈ nodes ]
	out_base_idcs = [ sum(num_inputs[1:k]) for k ∈ 1:n-1 ]
	out_base_idcs = [1; out_base_idcs[1:end] .+ 1]
	out_counts = zeros(length(out_base_idcs))

	inverseidces = zeros(Int64, length(outputs))
	for i ∈ 1:length(inputs)
		inverseidces[i] = out_base_idcs[delays[i].target] + out_counts[delays[i].target]
		out_counts[delays[i].target] += 1
	end
		
	DelayNetwork(inputs, outputs, delbuf, inverseidces, nodes, delays)
end


# -------------------- Parameters --------------------
n = 10 		# number of elements
p = 0.1 		
d_max = 200
graph = blobgraph(n, p, collect(1:d_max))

# ---------- Generate Network, Nodes and Delay Lines --------------------
verbose = true
noise = 0.0 	# probability of random firing
num_steps = 10

graph_test = [0 4 4 0; 0 0 4 0; 0 0 0 4; 4 0 0 0 ]

# outputs, inputs, delbuf, nodes, delays, inverseidces = delnetfromgraph(graph_test)
dn = delnetfromgraph(graph_test)

# simple state for each node
nodevals = zeros( length(dn.nodes) )
nodevals[1] = 1.0
nodevals[2] = 1.0

# run simulation
for j ∈ 1:num_steps
	#global inputs, outputs, inverseidces, delays, delbuf, op, nodevals
	global dn, nodevals

	for k ∈ 1:length(nodevals)
		for l ∈ 1:dn.nodes[k].num_out
			dn.inputs[dn.nodes[k].idx_in_to_out+l-1] = nodevals[k]   
			if rand() < noise
				dn.inputs[dn.nodes[k].idx_in_to_out+l-1] = 1.0
			end
		end
	end

	if verbose
		println("\nSTEP $j:")
		println("nodevals: $(nodevals)")
		println(dn.inputs |> buftostr, "\n")
	end
	advance(dn.inputs, dn.outputs, dn.invidx, dn.delays, dn.delaybuf)

	for d ∈ dn.delays
		vals = orderbuf(d, dn.delaybuf)
		verbose && println("($(d.source)) "*vals*" ($(d.target))")
	end

	verbose && println("\n", dn.outputs |> buftostr, "\n")
	
	nodevals = zeros(length(dn.nodes))
	for k ∈ 1:length(nodevals)
		for l ∈ 1:dn.nodes[k].num_in
			nodevals[k] += dn.outputs[dn.nodes[k].idx_out_to_in+l-1] 
		end
		nodevals[k] = nodevals[k] > 1.0 ? 1.0 : 0
	end

	verbose && println("############################################################")
end


end