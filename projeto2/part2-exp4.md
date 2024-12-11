# Steps

## 1
`/interface bridge port remove [find interface =ether9]`

`/interface bridge port add bridge=bridge11 interface=ether9`

`/ip address add address=172.16.1.11/24 interface=ether1`

`/ip address add address=172.16.11.254/24 interface=ether2`

## 2
tux13:
`route add -net 172.16.1.0/24 gw 172.16.10.254`

`route add -net 172.16.11.0/24 gw 172.16.10.254`

![alt text](images/part2_exp4/##2tux13.jpeg)

tux14:

`route add 172.16.1.0/24 gw 172.16.11.254`

- RC -> `/ip route add dst-address=172.16.10.0/24 gateway=172.16.11.253`

tux12:
`route add 172.16.1.0/24 gw 172.16.11.254`

`route add -net 172.16.10.0/24 gw 172.16.11.253`


## 3
tux13 -> tux14 (eth1)
`root@tux13: ping 172.16.10.254`

![alt text](images/part2_exp4/1314eth1.jpeg)

tux13 -> tux12
`root@tux13: ping 172.16.11.1`

![alt text](images/part2_exp4/1312.jpeg)


tux13 -> tux14 (eth2)
`root@tux13: ping 172.16.11.253`

![alt text](images/part2_exp4/1314eth2.jpeg)

tux13 -> RC
`root@tux13: ping 172.16.11.254`
![alt text](images/part2_exp4/tux13rc.jpeg)


## 4

### 4.1
```bash
sysctl net.ipv4.conf.eth1.accept_redirects=0
sysctl net.ipv4.conf.all.accept_redirects=0 
```
### 4.2
`route del -net 172.16.10.0/24 gw 172.16.11.253`
`route add -net 172.16.10.0/24 gw 172.16.11.254`

`route del -net 172.16.10.0/24 gw 172.16.11.254`
`route add -net 172.16.10.0/24 gw 172.16.11.253`

### 4.3
- Ping tux12:
![alt text](images/part2_exp4/ping12.jpeg)

### 4.4
- Capture:
![alt text](images/part2_exp4/capt12.jpeg)


### 4.5 | 4.6
- Traceroute tux13 in tux12 before change:
![alt text](images/part2_exp4/trace1.jpeg)

- Traceroute tux13 in tux12 after change
![alt text](images/part2_exp4/trace2.jpeg)

### Last step
```bash
sysctl net.ipv4.conf.eth1.accept_redirects=1
sysctl net.ipv4.conf.all.accept_redirects=1
```
`route del -net 172.16.10.0/24 gw 172.16.11.253`



## 5
`root@tux13: ping 172.16.1.10`
![alt text](images/part2_exp4/5.jpeg)

## 6
`/ip firewall nat disable 0`


## 7
![alt text](images/part2_exp4/7.jpeg)
