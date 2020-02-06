module DelNetSketch

using Plots

mutable struct Node
	# the lists should just be "pointers" -- but try once with eliminating
	# them to see if it enhances performance
	startidx_in::Int64
	num_in::Int64
	nodes_in::Array{Int64,1} 	# just for reference/construction
	startidx_out::Int64
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

# -------------------- Parameters --------------------
n = 3 		# number of elements
p = 0.1 		
d_max = 5


# Generate connectivity matrix
# delmat = rand(n,n) |> m -> map(x -> x < p ? 1 : 0,m)
# for k ∈ 1:n delmat[k,k] = 0 end
# numlines = sum(delmat)
# delmat = map(x -> x == 1 ? rand(1:d_max) : 0, delmat)
# deltot = sum(delmat)

# for testing
delmat = [0 1 0; 0 0 1; 1 0 0 ]
for k ∈ 1:n delmat[k,k] = 0 end
numlines = sum(delmat)
delmat .*= d_max
deltot = sum(delmat)

inputs = zeros(numlines)
outputs = zeros(numlines)
delbuf = zeros(deltot)

nodes = [Node() for _ ∈ 1:n] 
delays = Array{Delay, 1}(undef,numlines)

e_forw = []
e_revr = []


delcount = 1
startidx = 1
input_idx = 1
output_idx = 1
total = 0
for i ∈ 1:n
	global delcount
	global startidx
	global total
	global input_idx
	for j ∈ 1:n
		if delmat[i,j] != 0
			total += delmat[i,j]
			push!(e_forw, (i,j))				
			push!(e_revr, (j,i))				
			push!(nodes[i].nodes_out, j)
			nodes[i].num_out += 1
			push!(nodes[j].nodes_in, i)
			nodes[j].num_in += 1
			nodes[i].startidx_in = input_idx
			delays[delcount] = Delay(0, startidx, delmat[i,j], i, j)
			startidx += delmat[i,j]
			delcount += 1
			input_idx += 1
		end
	end
end
println("Sanity check -- the same?: $deltot, $total")

num_inputs    = [ nd.num_in for nd ∈ nodes ]
out_base_idcs = [ sum(num_inputs[1:k]) for k ∈ 1:n-1 ]
out_base_idcs = [1; out_base_idcs[1:end] .+ 1]
out_counts = zeros(length(out_base_idcs))

inverseidces = zeros(Int64, length(outputs))
for i ∈ 1:length(inputs)
	println(i)
	inverseidces[i] = out_base_idcs[delays[i].target] + out_counts[delays[i].target]
	out_counts[delays[i].target] += 1
end


# function advance(input, output, inverseidces, delays, delbuf)	
function advance(input, inverseidces, delays, delbuf)	
	output = zeros(length(input))
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
		outputs[ inverseidces[i] ] = delbuf[ delays[i].startidx + delays[i].offset ]	
	end
	output
end

orderbuf(delay, delbuf) = 
reverse([delbuf[delay.startidx + ((delay.offset + k) % delay.len) ]
		 for k ∈ 0:delay.len-1] ) |> v -> map(x -> x == 0.0 ? "-" : "*", v) |> prod


inputs[1] = 1.0
#delbuf[1] = 1.0
num_steps = 17
for j ∈ 1:num_steps
	global inputs, outputs, inverseidces, delays, delbuf
	println("----------------------------------------")
	# println(inputs, "\n")
	# println(delbuf)
	# println("\n", outputs, "\n")
	for d ∈ delays
		vals = orderbuf(d, delbuf)
		println(vals)
	end
	# advance(inputs, outputs, inverseidces, delays, delbuf)
	# (inputs, outputs) = (outputs, inputs)
	# outputs = advance(inputs, inverseidces, delays, delbuf)
	# inputs = outputs
	
	for i ∈ 1:length(inputs)
		delbuf[ delays[i].startidx + delays[i].offset ] = inputs[i]	
	end

	# advance buffer
	for i ∈ 1:length(inputs)
		#println(delays[i].startidx + delays[i].offset)
		delays[i].offset = (delays[i].offset + 1) % delays[i].len
		#println(delays[i].startidx + delays[i].offset)
	end

	#pull output
	for i ∈ 1:length(inputs)
		outputs[ inverseidces[i] ] = delbuf[ delays[i].startidx + delays[i].offset ]	
	end
	# inputs = zeros(length(inputs))
	inputs = copy(outputs)
end


end