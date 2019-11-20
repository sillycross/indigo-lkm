This is a utility tool that processes the action log of Indigo and generates data for training.

It reads input from /proc/indigo_training_output (where Indigo writes its action log into), 
and takes 3 additional parameters: (1) T, the required time series length, (2) expert_cwnd, 
the expert congestion window size (it assumes that all the log has the same expert_cwnd), 
and (3) the .npz file name to write the generated training dataset.
E.g. to parse data collected from a 20ms delay 12Mbits bw link (so its expert_cwnd = 40), 
with a required time series length of 1000, one should use: 

./format_output 1000 40 out.npz


Suppose the time series length is T, and the input vector of the NN has length m. 
The .npz file will contain two variables:

expert_actions: np.array([n, T], np.int32)
input_vectors: np.array([n, T, m], np.float32)

where n is the # of time series collected (one time series will be generated for each iperf run).


Time series with length less than T will be discarded with a warning. 
Time series with length larger than T will be truncated. 


