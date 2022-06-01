# network_project_team13

## Introduction
It is an application that provides video streaming service.
Streaming means delivering content from a server to a client over the Internet without downloading.
Therefore, in the case of video streaming, udp is used as the communication protocol, but we made a realiable udp that does not decrease the communication speed much under the condition that there are not many frames to send.

## How to install
### Prerequisites
ns-3 (version 3.29), Python, C++ compiler (clang++ or g++)

### Installation Order
1.  Download and build ns-3.
2.  Copy the files exactly into the folders of the ns-3. 
   (wscript in src->applications)
3.  Run `./waf` or `./waf build` to build the new application.  
   (If this is the first time running waf, you should do  `./waf configure --build-profile=debug`)

## How to use
- we have 2 cases  
  (1) p2p link for 1 client and 1 server  
  (2) wifi link for 1 client and 1 server

Run `./waf --run videoStream`.  
Run `./waf —run "videoStream --case=<case> --pktPerFrame=<packets per frame>"`

## Result
#### (1) default(p2p link, 100 packets per frame)
Run `./waf --run videoStream`.  
- first column: "time(sec)".  
- second column: "Number of frames currently available".
```
(...)
7	20
8	20
9	20
10	20
11	20
12	15
13	17
14	12
15	12
16	19
17	12
18	13
(...)
```


#### (2) p2p link, 50 packets per frame
Run `./waf --run "videoStream --case=1 --pktPerFrame=50"`. 

```
(...)
7	20
8	20
9	20
10	20
11	20
12	20
13	20
14	20
15	20
16	20
17	20
(...)
```

#### (3) wifi link, 50 packets per frame
Run `./waf --run "videoStream --case=2 --pktPerFrame=50"`. 

```
(...)
7	20
8	20
9	20
10	20
11	20
12	20
13	20
14	20
15	20
16	10
17	3
18	3
(...)
```



