@mainpage
# Example of obtaining "A" and SRV type DNS records via WiFi

(See the README.md file in the upper level Espressif 'examples' directory for more information about examples.)

This program demonstrates how to get information on the netif interface, the DNS assigned when
the IP address was assigned, and shows how to get an "A record" using the methodology shown
by Adam Dunkels

It is part of a series of programs

resolv_1_A        -- This was a first try to convert adam dunkels code to ESP32 uses jps
                      functions for htons and parses without using a data structure. It demonstrates
                      function callbacks when a UDP buffer is received and function callbacks
                      when an IP4 address is found. Code only works on A records. When doin This
                      routine, I became aware how much more efficient the struct method for parsing
                      received buffers was. I also used this to learn about what functions LWIP
                      provides. Tis file will only be used for historical purposes

resolv_2_A        -- This project builds on resolv_01. It removes the extra code I had written
                      and fully demonstrates getting A records using the Adam Dunkels method. You
                      would use this code if you wanted to implement a DNS resolver as envisioned by
                      Dunkels for A records only.

resolv_3_A+SRV     -- This projects adds the critical ability to get SRV records and introduces a
                      function res_query, which creates a query buffer, sends it to the DNS
                      server, waits for a reply, copies the buffer into a user supplied buffer, and
                      returns the length of the buffer. This project retains the working Adam
                      Dunkels style code (A records only). This project proves all the features
                      needed to replace the res_query function provided on unix and windows but
                      not available on the ESP32

sti_dns             -- This function demonstrates res_query with sti code. The functions can now be
                      included in other projects when res_query is required.
## How to use example

### Configure the project

```
idf.py menuconfig
```

* Set WiFi SSID and WiFi Password and Maximum retry under Example Configuration Options.

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output
Note that the output, in particular the order of the output, may vary depending on the environment.

Console output if station connects to AP successfully:
```

I (2090) wifi station: .Information on Netif connection
I (2100) wifi station: ...Netif is running
I (2100) wifi station: ...Current IP from netif      : 192.168.1.20
I (2110) wifi station: ...Current Hostname from netif: espressif
I (2120) wifi station: ...Name Server Primary (netif): 192.168.1.1
I (2120) wifi station:

I (2130) wifi station: .Initialize the Resolver
I (2130) resolv init : ...udp connected to: 8.8.8.8
I (2140) wifi station:
I (2140) wifi station: ...Start of res_query for A records
I (2150) res_query   : ...query sent to DNS server
I (2550) wifi station: ...length of returned buffer is 49
I (2550) print_buf   : ....1 Hex in received buffer   : 0
I (2550) print_buf   : ....2 Letter in received buffer: c
I (2550) print_buf   : ....3 Hex in received buffer   : 81
I (2560) print_buf   : ....4 Hex in received buffer   : 80
I (2560) print_buf   : ....5 Hex in received buffer   : 0
I (2570) print_buf   : ....6 Hex in received buffer   : 1
I (2580) print_buf   : ....7 Hex in received buffer   : 0
I (2580) print_buf   : ....8 Hex in received buffer   : 1
I (2590) print_buf   : ....9 Hex in received buffer   : 0
I (2600) print_buf   : ....10 Hex in received buffer   : 0
I (2600) print_buf   : ....11 Hex in received buffer   : 0
I (2610) print_buf   : ....12 Hex in received buffer   : 0
I (2610) print_buf   : ....13 Hex in received buffer   : 4
I (2620) print_buf   : ....14 Letter in received buffer: x
I (2630) print_buf   : ....15 Letter in received buffer: m
I (2630) print_buf   : ....16 Letter in received buffer: p
I (2640) print_buf   : ....17 Letter in received buffer: p
I (2640) print_buf   : ....18 Hex in received buffer   : 7
I (2650) print_buf   : ....19 Letter in received buffer: d
I (2660) print_buf   : ....20 Letter in received buffer: i
I (2660) print_buf   : ....21 Letter in received buffer: s
I (2670) print_buf   : ....22 Letter in received buffer: m
I (2680) print_buf   : ....23 Letter in received buffer: a
I (2680) print_buf   : ....24 Letter in received buffer: i
I (2690) print_buf   : ....25 Letter in received buffer: l
I (2690) print_buf   : ....26 Hex in received buffer   : 2
I (2700) print_buf   : ....27 Letter in received buffer: d
I (2710) print_buf   : ....28 Letter in received buffer: e
I (2710) print_buf   : ....29 Hex in received buffer   : 0
I (2720) print_buf   : ....30 Hex in received buffer   : 0
I (2720) print_buf   : ....31 Hex in received buffer   : 1
I (2730) print_buf   : ....32 Hex in received buffer   : 0
I (2740) print_buf   : ....33 Hex in received buffer   : 1
I (2740) print_buf   : ....34 Hex in received buffer   : C0
I (2750) print_buf   : ....35 Hex in received buffer   : C
I (2760) print_buf   : ....36 Hex in received buffer   : 0
I (2760) print_buf   : ....37 Hex in received buffer   : 1
I (2770) print_buf   : ....38 Hex in received buffer   : 0
I (2770) print_buf   : ....39 Hex in received buffer   : 1
I (2780) print_buf   : ....40 Hex in received buffer   : 0
I (2790) print_buf   : ....41 Hex in received buffer   : 0
I (2790) print_buf   : ....42 Hex in received buffer   : 7
I (2800) print_buf   : ....43 Hex in received buffer   : 7
I (2800) print_buf   : ....44 Hex in received buffer   : 0
I (2810) print_buf   : ....45 Hex in received buffer   : 4
I (2820) print_buf   : ....46 Letter in received buffer: t
I (2820) print_buf   : ....47 Hex in received buffer   : CB
I (2830) print_buf   : ....48 Hex in received buffer   : 3
I (2840) print_buf   : ....49 Hex in received buffer   : FD
I (2840) wifi station: ...End res_query for type A records
I (3850) wifi station:
I (3850) wifi station: ...Start of res_query for SRV records
I (3850) res_query   : ...query sent to DNS server
I (4250) wifi station: ...length of res_query returned buffer 81
I (4250) print_buf   : ....1 Hex in received buffer   : 0
I (4250) print_buf   : ....2 Letter in received buffer: c
I (4250) print_buf   : ....3 Hex in received buffer   : 81
I (4260) print_buf   : ....4 Hex in received buffer   : 80
I (4270) print_buf   : ....5 Hex in received buffer   : 0
I (4270) print_buf   : ....6 Hex in received buffer   : 1
I (4280) print_buf   : ....7 Hex in received buffer   : 0
I (4280) print_buf   : ....8 Hex in received buffer   : 1
I (4290) print_buf   : ....9 Hex in received buffer   : 0
I (4300) print_buf   : ....10 Hex in received buffer   : 0
I (4300) print_buf   : ....11 Hex in received buffer   : 0
I (4310) print_buf   : ....12 Hex in received buffer   : 0
I (4310) print_buf   : ....13 Hex in received buffer   : C
I (4320) print_buf   : ....14 Hex in received buffer   : 5F
I (4330) print_buf   : ....15 Letter in received buffer: x
I (4330) print_buf   : ....16 Letter in received buffer: m
I (4340) print_buf   : ....17 Letter in received buffer: p
I (4350) print_buf   : ....18 Letter in received buffer: p
I (4350) print_buf   : ....19 Hex in received buffer   : 2D
I (4360) print_buf   : ....20 Letter in received buffer: c
I (4360) print_buf   : ....21 Letter in received buffer: l
I (4370) print_buf   : ....22 Letter in received buffer: i
I (4380) print_buf   : ....23 Letter in received buffer: e
I (4380) print_buf   : ....24 Letter in received buffer: n
I (4390) print_buf   : ....25 Letter in received buffer: t
I (4390) print_buf   : ....26 Hex in received buffer   : 4
I (4400) print_buf   : ....27 Hex in received buffer   : 5F
I (4410) print_buf   : ....28 Letter in received buffer: t
I (4410) print_buf   : ....29 Letter in received buffer: c
I (4420) print_buf   : ....30 Letter in received buffer: p
I (4430) print_buf   : ....31 Hex in received buffer   : 7
I (4430) print_buf   : ....32 Letter in received buffer: d
I (4440) print_buf   : ....33 Letter in received buffer: i
I (4440) print_buf   : ....34 Letter in received buffer: s
I (4450) print_buf   : ....35 Letter in received buffer: m
I (4460) print_buf   : ....36 Letter in received buffer: a
I (4460) print_buf   : ....37 Letter in received buffer: i
I (4470) print_buf   : ....38 Letter in received buffer: l
I (4470) print_buf   : ....39 Hex in received buffer   : 2
I (4480) print_buf   : ....40 Letter in received buffer: d
I (4490) print_buf   : ....41 Letter in received buffer: e
I (4490) print_buf   : ....42 Hex in received buffer   : 0
I (4500) print_buf   : ....43 Hex in received buffer   : 0
I (4510) print_buf   : ....44 Hex in received buffer   : 21
I (4510) print_buf   : ....45 Hex in received buffer   : 0
I (4520) print_buf   : ....46 Hex in received buffer   : 1
I (4530) print_buf   : ....47 Hex in received buffer   : C0
I (4530) print_buf   : ....48 Hex in received buffer   : C
I (4540) print_buf   : ....49 Hex in received buffer   : 0
I (4540) print_buf   : ....50 Hex in received buffer   : 21
I (4550) print_buf   : ....51 Hex in received buffer   : 0
I (4560) print_buf   : ....52 Hex in received buffer   : 1
I (4560) print_buf   : ....53 Hex in received buffer   : 0
I (4570) print_buf   : ....54 Hex in received buffer   : 0
I (4570) print_buf   : ....55 Hex in received buffer   : 7
I (4580) print_buf   : ....56 Hex in received buffer   : 7
I (4590) print_buf   : ....57 Hex in received buffer   : 0
I (4590) print_buf   : ....58 Hex in received buffer   : 17
I (4600) print_buf   : ....59 Hex in received buffer   : 0
I (4600) print_buf   : ....60 Hex in received buffer   : A
I (4610) print_buf   : ....61 Hex in received buffer   : 0
I (4620) print_buf   : ....62 Hex in received buffer   : 0
I (4620) print_buf   : ....63 Hex in received buffer   : 14
I (4630) print_buf   : ....64 Letter in received buffer: f
I (4640) print_buf   : ....65 Hex in received buffer   : 4
I (4640) print_buf   : ....66 Letter in received buffer: x
I (4650) print_buf   : ....67 Letter in received buffer: m
I (4650) print_buf   : ....68 Letter in received buffer: p
I (4660) print_buf   : ....69 Letter in received buffer: p
I (4670) print_buf   : ....70 Hex in received buffer   : 7
I (4670) print_buf   : ....71 Letter in received buffer: d
I (4680) print_buf   : ....72 Letter in received buffer: i
I (4680) print_buf   : ....73 Letter in received buffer: s
I (4690) print_buf   : ....74 Letter in received buffer: m
I (4700) print_buf   : ....75 Letter in received buffer: a
I (4700) print_buf   : ....76 Letter in received buffer: i
I (4710) print_buf   : ....77 Letter in received buffer: l
I (4720) print_buf   : ....78 Hex in received buffer   : 2
I (4720) print_buf   : ....79 Letter in received buffer: d
I (4730) print_buf   : ....80 Letter in received buffer: e
I (4730) print_buf   : ....81 Hex in received buffer   : 0
I (4740) wifi station: ...End res_query for SRV records
I (4750) wifi station:

I (4750) wifi station: .Begin gethostbyname
I (4940) wifi station: ...Gathering DNS records for xmpp.dismail.de
I (4940) wifi station: ...Address No. 0 from DNS: 116.203.3.253
I (4940) wifi station: Done with connection... Now shutdown handlers
