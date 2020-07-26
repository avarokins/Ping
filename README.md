# Ping
My implementation of the Command Line ping application with user adjustable Time To Live(ttl).

As a part of my intro to Systems Programming class, I learnt about the foundational technologies that are behind the internet. One program essential in debugging web apps or APIs is undoubtedly Ping. I tried to replicate the functionality of Ping applicaiton that is bundled with almost every shell.

The program, written in C, opens a RAW socket and repeatedly sends ICMP echo requests to the specified domain. I also added a DNS resolver so that the user can enter human-friendly domain names. The user can also specify a custom TTL to limit the number of router hops the request has to make. The program provides a summary page whene execution is stopped by the user. Visit GitHub Repository

Having a custom TTL option gave me the idea to implement the functionality of traceroute, to give a user the route their request takes to reach it's destination, which is one of the projects I am currently working on.
