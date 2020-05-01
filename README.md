# Distributed-Processing-Project

Problem description:

"Bad Guys are roaming the world and doing bad things, while Good Guys are roaming the world and fixing bad things done by Bad Guys. Because goodness and evil strongly depends on point of view, these days both Bad Guys and Good Guys got closed in psychiatric hospital. To avoid patients dying of boredom, they are allowed to roam the hospital and destroy/repair toilets or move flowerpots to shaded places and back. It means we have two types of processes - number Z of Bad Guys and number D of Good Guys (Z doesn't have to be equal do D). Bad guys first decide if they want to destroy one of the T toilets or move to dark place one of O flowerpots. Obviously broken toilets can't be broken once again and flowerpots which are in dark place can't be moved to even darker place. Meanwhile Good Guys are repairing broken toilets and moving flowerpots back to sunny places. Only one Guy is allowed to manipulate the resource at a time."

This project contains Lamport Algorithm variation, which solves (hopefully) problem given above.

Run with:
./run.sh <NUMBER_OF_PROCESSES> <NUMBER_OF_GOOD_GUYS> <NUMBER_OF_BAD_GUYS> <NUMBER_OF_TOILETS> <NUMBER_OF_PLANTS>
