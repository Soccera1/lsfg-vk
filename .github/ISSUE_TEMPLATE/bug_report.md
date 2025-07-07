---
name: Bug report
about: Report a bug (if lsfg-vk does work, but not as expected)
title: "[BUG] Explain your bug"
labels: bug
assignees: ''

---

**Describe the bug**
A clear and concise description of what's happening is and, if not immediately obvious, what's supposed to happen instead.

**To Reproduce**
Steps to reproduce the behavior:
1. Open '...'
2. Do '...'
3. Notice '...'

**Screenshots/Videos**
If applicable, add screenshots to help explain your problem.

**System information**
What Linux distro are you on? What driver version are you using? What's in your machine? 
Anything that could be relevant.

**Verbose log messages**
(you may skip this step if not relevant))

Grab the latest debug build (change "Release" to "Debug" in build parameters) and launch your game with these environment variables set:
`LSFG_LOG_FILE=lsfg.log LSFG_LOG_DEBUG=all VK_LOADER_DEBUG=all`
Pipe the application output to a file (`2>&1 | tee app.log` at the end of the command)

Upload both xxxx_lsfg.log and app.log.

Open up a terminal and type `vulkaninfo -o vulkan.txt`, this will create a file called vulkan.txt. Upload it as well.
