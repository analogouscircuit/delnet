#export periodicspiketrain, sparsepoisson, densepoisson, channelscatter, saveinput, loadinput, show

struct Spike
	i::UInt64
	t::Float64
end

Base.show(io::IO, s::Spike) = print(io, "Neuron: $(s.i)\tTime: $(s.t)")

function periodicspiketrain(f, dur, fs; magnitude=20.0)
	N_pat = Int(round(dur * fs))
	dn_pat = Int(round(fs/f))
	train = zeros(N_pat)
	idx = 1
	while idx <= N_pat
		train[idx] = magnitude 
		idx += dn_pat
	end
	train
end


exponentialsample(λ) = -log(rand()) / λ


function sparsepoisson(λ, dur)
	ts = Array{Float64, 1}(undef, 0)
	t = 0.0
	while t < dur
		push!(ts, t)
		t += exponentialsample(λ)
	end
	ts
end


function densepoisson(λ, dur, fs; magnitude=20.0)
	ts = sparsepoisson(λ, dur)
	output = zeros(Int64(round(dur*fs))+1)
	for t ∈ ts
		output[Int64(round(t*fs))+1] = magnitude 
	end
	output
end


function channelscatter(times::Array{T,1}, available_channels) where T<:Number
	n = length(times)
	spikes = Array{Spike, 1}(undef, n)
	if length(available_channels) > n
		channels = shuffle(available_channels)[1:n]	
		for i ∈ 1:n
			spikes[i] = Spike(channels[i], times[i])
		end
	else
		for i ∈ 1:n
			spikes[i] = Spike(rand(available_channels), times[i])
		end
	end

	spikes
end


function saveinput(input, trialname)
	open(trialname * "_input.bin", "w") do f
		write(f, length(input)) 	# write an Int64 (long int)
		write(f, input) 			# write an array of Float64 (double)
	end
end


function loadinput(trialname)
	open(trialname * "_input.bin", "r") do f
		len = read(f, Int64)
		input = Array{Spike, 1}(undef, len)
		input = read(f, input)
		return input
	end
end


function inputrasterlines(t1, t2, spikes, inputblock::Array{Spike,1}, inputtimes, fs)
	inputtimes = filter(x -> t1 < x < t2, inputtimes)

	# Spike Raster
	idx1 = searchsortedfirst(spikes[:,1], t1)
	idx2 = searchsortedlast(spikes[:,1], t2)
	numneurons = maximum(spikes[idx1:idx2,2])
	p1 = scatter(spikes[idx1:idx2,1], spikes[idx1:idx2,2],
				markersize=3.0,
				markeralpha=0.5,
				markerstrokewidth=0.1,
				markercolor=:blue,
				legend=:none,
			    xlims=(t1,t2),
				reuse=false)

	# Lines on raster
	for t ∈ inputtimes 
		plot!(p1, [t,t], [0,numneurons], lc=:red, ls=:dash, lw=1.25)
	end

	# Show input
	ts = range(t1, t2, step=1.0/fs);
	for inputtime ∈ (inputtimes .- t1)
		scatter!(p1, [s.t for s ∈ inputblock] .+ inputtime, [s.i for s ∈ inputblock],
				 markersize=5.0,
				 markeralpha=0.1,
				 markercolor=:red)
	end

	return p1
end
