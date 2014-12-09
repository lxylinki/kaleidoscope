#!/bin/bash
iptables -A INPUT -p tcp -m tcp --dport 5201 -j ACCEPT
iptables -A INPUT -p udp -m udp --dport 20121 -j ACCEPT
iptables -A INPUT -p udp -m udp --dport 20132 -j ACCEPT
