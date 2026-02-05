# Projects 2026
## for iot projects
A folder structure that allows you to have **devCores** versions providing connectivity and messaging, **gadgets** as a collection of sensor/actuator ESP code and definitions, and **projects** that are defined by config files and choice of devCore and Sensor class, you need to create a `platformio.ini` that:

The intent is for this to be expandable beyond just iot/ to include a folder structure thea supports the evolution of servers, databases and web-apps both for use in iot projects and other projects.

It will also be a record of deployed projects, what board each devid is in, what api is available, what kind of records is it storing on a database, what apps are using it and where are thos apps hosted from, what server is it talking to, what is the pcb it is in or the case that was printed for it. It will refer to other files both in this projects folder or in WSL or on remote servers.