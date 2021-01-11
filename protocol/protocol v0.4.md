# DAENet

## Question:

1. Are there any differences compared to original Chord failure model? and what's the influence it brought to upper layer applications?
2. What's the dialing protocol?
3. Should there be a 'finish-tag' exchanging after completion of serving?
4. How to handle extreme cases in 'fail-resend' recovery model? (Receiver got ACK but sender doesn't get the content package)

## Answer:

Q1. No, totally the same failure-recovery model compared to Chord. Because a node failure / departure / Dos can only slow down the rate of convergence (with a smaller or larger step), rather than change the pattern of messaging. 
- For an internal message failure, specifically for the failure in contacting with successor node (i.e., stabilize()). After a timeout, the node that discovers the failure will flush its
finger table with a state-of-art, none-failure identifier from its successor list. Later on, periodically called operations will correct the successor anyway. And for the updating of successor list, it should be included in fix_fingers() with the same updating rate of finger table.
- For an internal message failure, specifically for failure in finding best predecessor (i.e., closest_preceding_code()), the search will proceed in both the finger table and the successor list.
- For an external message failure, specifically for the failure in contacting with successor node(i.e., last hop to dead drop node), the routing shouldn't be redirected.
A failure invoice should be sent to message sender.
- For an external message failure, specifically for failure in finding best predecessor (i.e., closest_preceding_code()), the search will proceed in both the finger table and the successor list. (same as internal)

Details of failure handling will be introduced later.

Q2. The dialing protocol is activated at the beginning of a communication. From
the perspective of the application layer, a client should first look for the key of service provider, now both the service provider and the service requester knows the key so that they can further build up reliable connections. Basically, all DAENet users (including service providers & service requesters) will agree on a map function integrated in SGX, this function is used to determine the dead drop location for initializing a conversation. Now, a service requester sends a request message to dead drop. In the dead drop, there is a 'service-queue', filled by attested service providers' IDs and service meta-data (e.g., the key of service provider). As long as a requesting message (by checking tag) arrives, the dead drop shall
send two messages to two-sides. Now the service requester receives an ACK and provider receives an service request. Next, the service provider will:
- Register the key of service requester.
- According to service type, generate shared secret and configurations (e.g., expected order of package, transmission rate, sliding window size, time-out ...)

Sending these back to the dead drop, swapping the content and sending back to two-sides. Next, if the service requester agrees on the messaging configurations (i.e., orders, rate, ...), it will sends a 'start service' message back to dead drop. And by receiving this message, the service provider will start the service at once.

Details of dialing protocol will be introduced later.

Q3 & Q4. Yes, a 'Fin' message should be exchanged on two sides similar to the TCP 4-way handshaking.

Details of the failure protocol will be introduced next.

# Protocol

## Bootstrap Phase

Both clients and servers(i.e., service providers) join the system as a *node* asynchronously. Each activates the same sequence of bootstrap methods, they are: *join()*->*stabilize()*->*fix_fingertable()*. By the time complete *fix_fingertable()* operation, a node could start serving as either a client or a server.

## Establish Reliable Connection Through Dialing

Suppose Alice and Bob both joined in the DAENet system, and as a client, Alice would like to fetch a secret which is owned by an unknown provider Bob. Since Bob has registered his service in a third-party application by providing his *DAE Key*. Alice then goes to obtain the *DAE Key* of Bob in order to establish a reliable connection.

Now the dialing protocol is activated. Since all DAENet users (including service providers & clients) will agree on a map function integrated in SGX, Alice and Bob both figuring out a same location for dialing, which is called the *dead drop*. As a service provider, Bob registered his service in the dead drop by storing his service ID in a *service queue*. As long as the dead drop parses Alice's coming dialing request, the dead drop sends two messages to Alice and Bob:
- To Alice: The service provider has received your request, you should send another request to the dead drop for a configuration file.
- To Bob: A client (unknown) is requesting for fetching your file. If Bob accepts the request, he then establishes a configuration file for transmission, including: *session ID*, *shared secret*, *transmission rate*, *time_out*, *order of slicing*, *size of window* and some other metadata. And he sends this package to the dead drop. Otherwise, send a *Reject* message to the dead drop.

The dead drop swaps the content, sends an ACK message to Bob and a configuration message to Alice. Bob receives the ACK and knows that Alice has received the config/Rej. If Bob denies the request, the service is done. Otherwise, Bob sends a message to the dead drop again, waiting for Alice's permission. In a time-out, if Bob receives the ACK from the dead drop, then the connection channel is established successfully. Alice also receives an ACK from the dead drop, he can use the configurations specified in last package to fetch the file.

This process acts as a 3-way handshaking.

## Failure Handling 

Similar to Chord, DAENet handles network layer failures by maintaining correct successor node or at least give a better option. By utilizing *successor list*, DAENodes can always provide better options when successor node fails, departs or even suffers from a severe DOS attack, at the cost of acceptable extra bandwidth.

Basically, according to the destination (**dead drop / relay**), there are two kinds of failures that could happen in DAENet. And by observing the type of the message (**internal / external**), there are totally four kinds of failures that could appear in DAENet. How these failures could happen and the dealing approaches are introduced next:

- For an **internal** message failure, specifically for the failure in contacting with immediate successor node (i.e., **dead drop**). This failure happens in *stabilize()* function. After a timeout, the node that discovers the failure will flush its
finger table with a state-of-art, none-failure identifier from its successor list. Later on, periodically called operations (i.e., *check_predecessor(), stabilize()*) will correct the successor anyway. Whenever a member in the successor list fails, the successor list should got updated. And for the updating of successor list, it should be included in *fix_fingers()* with the same updating rate of finger table. By using that method, we get a strong assumption of DAENet: DAENet could work well even in an unstable network environment, where participants joins and leaves with the same probability.

- For an **internal** message failure, specifically for failure in finding best predecessor (i.e., **relay**), the search will proceed in both the finger table and the successor list. This failure happens in *closest_preceding_code()*.

- For an **external** message failure, specifically for the failure in contacting with successor node(i.e., **dead drop**), the routing shouldn't be redirected.
A failure invoice should be sent to message sender.  This failure happens in *find_successor()*.

- For an **external** message failure, specifically for failure in finding best predecessor (i.e., **relay**), the search will proceed in both the finger table and the successor list. (same as internal). This failure happens in *closest_preceding_code()*.

## Anonymous Transmission 

Consider the case: Bob slices the file into **10** independent packages with a sliding window size of **3**. And now we name the four windows as 'a', 'b', 'c', 'd', each of which carries several slices of the file. For example, window 'b' carries three slices: '4', '5', '6' and window 'd' carries one slice '10':

     Packets ready to send: | 1 2 3 | 4 5 6| 7 8 9 | 10 |
         
     Name of windows:           a       b      c     d

Denotes that a normal transmission includes two steps: packaging a message and sending out. The format of transmission could be expressed as:
***User1 > package (made by User1) > User2.***

** **IMPORTANT:** ** Packets are within a standard format, but differs in tags because of different types of senders. Reminding that network packets are of the same format, the example below shows only the typical tags of different user types and ignores the same part. 

Now, since the configuration has been agreed on by two sides: Alice (denoted as A) and Bob (denoted as B), they communicate in an anonymous manner through dead drop i (Denoted as Di). The normal case of transmission without exceptions should be like this:

### Slice a: 
    | B > 1/T > D1 ; B > 2/T > D2 ; B > 3/T > D3             |
                                                             | ------> F1    
    | A > nil/1 D1 ; A > nil/2 > D2 ; A > nil/3 > D3         |

    | D1:  D1 > 1/T > A  ;  D1 > ACK/nil > B 
      D2:  D2 > 2/T > A  ;  D2 > ACK/nil > B 
      D3:  D3 > 3/T > A  ;  D3 > ACK/nil > B 
           ____________     ________________           
                F2                 F3


### Slice b & c:
Same as Slice a.

### Slice d: 
    | B > 10/F > D10 

    | A > nil/10 > D10

    | D10: D10 > 10/F > A ; D10 > ACK/nil > B

    | A > fin_A > D11                                         |
                                                              |
    | B > fin_B > D11                                         |-------> F4
                                                              |
    | D11: D11 > fin_B > A ; D11 > fin_A > B                  |
          

Seen from the operation, the *fin_i* tag is passed to client A together with the last package. After that, A & B goes to the final dead drop denoted as D11 to exchange the *fin_i* tag. After the exchanging, A is aware of B's 'completing status', B finds out that A knows B's 'completing status'. This session now comes to an end.

Similar to TCP 4-way handshaking in disconnecting process, a *fin_i* tag only represents that i is willing to disconnect the session. So the exchanging is necessary. Nevertheless, different from typical 4-way handshaking in TCP, DAENet's communication is based on swapping (exchanging) rather than a simplex communication. This helps to save the bandwidth cost but incurs a problem of inconsistency. Next, we divide the problem into four categories of failures (denoted as F1 ~ F4 in the example) for discussion.


## Exception Handling

### F1:

The underlying failure protocol ensures the reachability of an application message as long as the instant messaging destination **is not a dead drop**. Any failure that is encountered during a messaging to a **relay** will be redirected, and will finally reach the destination with a relative slower convergence rate. So here, under the circumstance of F1, there are two cases:
- Fails not in last hop: 
  
  This would get easily figured out by underlying redirecting protocol introduced earlier.

- Fails in last hop:

  According to the failure protocol, once the hop to the dead drop fails, the sender will send back a message to the initial message sender (within several hops), to indicate the failure. In this case, either A or B fails will cause the same result, because one side will receive a failure message while the other side will pose a time_out. Now, A & B agrees on one fact according to their observation: the transmission for slice '1' is failed. As a response, they will try to restart the failed transmission of slice '1' through next proper dead drop. 

  Since DAENet is an asynchronous messaging system, and the sliding window only moves when an ACK arrives (for B) or a content package arrives (for A). Although a failed messaging would get sent out with higher priority, there still exists extreme cases of inconsistent arrival. As a result, **the target dead drop for A & B could be different**. Totally two cases here:
  - Consistent transmission rate with simultaneous arrival of failure invoice. In this case, A and B will swap the operation for A and B as follows:

           | B > 1/T > D4 ; B > 4/T > D5 ; B > 5/T > D6

           | A > nil/1 >D4 ; A > nil/4 > D5 ; A > nil/5 > D6

    In this case, all request and package matches exactly what they need, swapping the content and make a deal, everything goes on well. The exception is handled.

  - Inconsistent transmission rate with non simultaneous arrival of failure invoice. As an example, B receives the ACK of slice '2' earlier than the failure invoice of slice '1', while A just receives the failure requesting invoice of slice '1'. They will send different jobs to the next dead drop (D4), the case will be like:

           | B > 4/T > D4 

           | A > nil/1 > D4

    , where B sends slice '4' to D4 and A wants to fetch slice '1' at D4. The inconsistency happens and the inconsistency type is: **Order(sender) > Order(client)**. The core insight of dealing with such inconsistency is to let the receiver/client A takes away the state-of-art content package and at the same time remind B of the loss of previous slice.

    The details of operations in handling this case will be introduced later, since this type of exception is the same as F2. And for the other inconsistency type: **Order(sender) < Order(client)**, this type of exception is the same as F3 and will introduced later as well.
  
### F2:

When a dead drop receives two messages with a same session, and also discovers the inconsistency of order: **Order(sender) > Order(client)** in this case. This could happen when a message from dead drop to A fails and a message to B arrives. In this scenario, the client A doesn't receive the message in a bound configured in configuration file. When A's sliding window moves, A should immediately send out a request for previous lost slice. Now in the dead drop, the expected slice isn't what A want. The dead drop node observes this condition, and sends back new slice to A. Meanwhile, the dead drop sends back a message with ACK with current slice's ID and a reminder for lost slice. After this transmission, since B is reminded to send lost message, A is expected to get the lost slice in a new dead drop. Assuming A lost the message from D1, the the process operates as follows:

    | A > nil/1 > D4

    | B > 4/T > D4

    D4 observes that Order(A) < Order(B), realizing that A loses proper slice:
    | D4: D4 > 4/T > A ; D4 > ACK/1 > B

    A gets new slice '4'; B knows that A hasn't get slice '1'; A & B then goes to a new dead drop to swap lost slice '1':
    | A > nil/1 > D7 ; B > 1/T > D7

 Upon the reaction of F2, chances are that the message from D4 to A fails again, in that case, A loses slice '4' together with slice '1' lost before. Now, A only knows about the loss of slice '1', but he doesn't know that slice '4' is also lost. On the other hand, from B's perspective, slice '1' will be resent later but slice '4' has been confirmed. Since A doesn't know that slice '4' is lost, there will be no request for slice '4' and it remains unknown to A from then on. To solve this problem, we use an **order checker (OC)** to check for lost message in an asynchronous environment.

 Most exceptions can be handled without OC, because lost slices could get fetched through carrying a signal in ordinary packets. Unfortunately, a '**failure in failure**' could result in a lost of critical information, such as the lost of slice '4' in this example. OC targets at such kind of failures, and runs periodically in a client. Since the configuration has pointed out the expected order in configuration file, OC will check whether there are lost slices when a client receives a */F* packet. This happens in F4 when a slice with tag *fin_i* arrives.

 F4 is introduced earlier to F3.

### Order Checker (OC) & F4:

In a normal case, when A/B ends up fetching/sending, they will exchange a *fin_i* tag to indicate the ending of this session. 

However, within such an asynchronous environment,it's possible that client A could incur some failures and misses proper slices. Follow the example, A loses slice '4' without knowing about that. After A & B finally exchanges the ending packet tagged as */F*, as is shown here, OC is activated.

    | B > 10/F > D10 

    | A > nil/10 > D10

    | D10: D10 > 10/F > A ; D10 > ACK/nil > B

In normal case, A & B would swap the *fin_i* tag to agree on the end up like: 

    | A > fin_A > D11                                         
                                                              
    | B > fin_B > D11                                         
                                                            
    | D11: D11 > fin_B > A ; D11 > fin_A > B             

Now that A calls the OC and checks the received order of slices, finding out that in a range of 10 (because slice '10' is tagged as */F*, no more slices will be sent), the slice '4' is missed, A then sends the missing slice ID to dead drop.

    | A > nil/4 > D11                                         
                                                              
    | B > fin_B > D11                                         
                                                            
    | D11: D11 > nil/T > A ; D11 > ACK/4 > B     

D11 finds out an inconsistency: the service provider B sends an *fin_i* tag while client A still requests a slice. D11 deals with this case as an exception: A has lost something by *failure in failure*. D11 will negotiates this case by asking each other to exchange at D12. After a D12 exchanging, B receives the ACK for slice '4', and again B sends a *fin_i* message to D13, waiting for confirmation.

For A, OC now outputs no lost slices. A then sends *fin_i* to D13, shaking hands and ending current conversation.

**If more than one slices are lost**, the packet would bring out a list instead of *nil*:

    Only one lost slice:
    | A > nil/4 > D11 

    More than one lost slices:
    | A > list[slices]/4 > D11

### F3:
In the above example, F3 happens when A receives slice '1' but B doesn't receive the ACK for slice '1'. A is going to fetch slice '4' and finds out that the receiving packets is with a *nil/T* tag. This indicates a failure of fetching slice '4' and A should continues fetching that slice with next dead drop.

    | A > nil/4 > D4

    | B > 1/T > D4

    D4 observes that Order(A) > Order(B), realizing that B missed an ACK for proper slice:
    | D4: D4 > nil/T > A ; D4 > ACK/nil > B

    A would goes to next dead drop for slice '4'. And from the perspective of B, slice '1' is 'well' received, B then needs to send next slice '4':
    | A > nil/41 > D7 ; B > 4/T > D7


# Core Concept of Protocol

- A failed slice owns higher priority compared to 'ready-to-send' slices.
- No matter what role a node plays, whenever encounters a failure, the only thing it has to do is to resend a same request to next proper dead drop.
- Dead drop plays an important role in swapping. A dead drop needs to compare and judge what kind of message it should make to reply to receivers. There are totally five cases:
    - The order from client/receiver is the same as server/sender.
    - The order from client/receiver is smaller than server/sender.
    - The order from client/receiver is bigger than server/sender.
    - Received two messages with 'fin_i' tag.
    - One with 'fin_i' tag and the other is not.

# Variables

Here are the variables.

