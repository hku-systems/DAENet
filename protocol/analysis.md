# DAE Protocol / Security Analysis

## Likelihood Analysis

Expected difference in likelihood

eps = | log(p_0) / log(p_1) |

We mark output of two sender two challenge sender S0 and S1 at T0. All other clients send and mix messages. We add tags to the two message, and track the output probability using a in-out transformation theorem (theorem 1 in Loopix). (What about DAE's theorem?) At time T1 (receiver or sender reply for dae, or last hop in Loopix), we compute an eps to evaluate the likelihood of difference.

Note that this evaluation shows only a conservative merit of an anonymous network: the payload messages are identified by attackers and they can identify two of the potential senders (out of n).

Two figure for evaluation: parameter sensibility;
For Loopix: number of layers/obfuscation.
For DAE: shuffle number + send rate (and more?)

## Entropy Analysis


## Message Drop Security Analysis
When an attacker drops message uniformly and observe if there are reply back, is the attack feasible (the message can be dropped before or after message swapped). 
We can simplify the attack: for a number of time, only one message is sent from two clients A, B. Then blocking one message can know if A or B is the sender of the message (when a malicious receiver is assumed). This attack is too simplified.

In practice, attackers can only block message uniformly (to some specific nodes or nodes in one area). Then, they
observe if the receiver get the message. If the receiver get all message, then the node is the sender. 
(Can DC-Net and Karaoke handle these kinds of problems?)

We can handle this problem by checking the returned rate of loop messages. Assume a package loss rate K, clients should receive K * totak_msg. If the number of msg is less,
then this client is under attack. f(K) represents the percentage of untrusted nodes (directly connected to the sender).



## Problems
1. is the average latency different for node pairs with different distances?
2. What if two node emit message in different rate?
3. What if a node drop the message?
4. Will the context of connection break the anonymity?