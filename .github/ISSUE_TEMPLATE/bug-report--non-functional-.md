---
name: Bug report (non-functional)
about: Report a bug (if lsfg-vk is not working at all).
title: "[BUG] Explain your bug"
labels: bug
assignees: ''

---

<!-- This template is for bugs that make lsfg-vk fully non-functional. This includes crashes and the game simply not being hooked properly -->

**Describe the bug**
A clear and concise description of what's happening is and, if not immediately obvious, what's supposed to happen instead.

**To Reproduce**
Steps to reproduce the behavior:
1. Open '...'
2. Move/Resize '...'
3. See error

**Screenshots/Videos**
If applicable, add screenshots to help explain your problem.

**System information**
What Linux distro are you on? What driver version are you using? What's in your machine? 
Anything that could be relevant.

**Verbose log messages**
Grab the latest debug build (change "Release" to "Debug" in build parameters) and launch your game with these environment variables set:
`LSFG_LOG_FILE=lsfg.log LSFG_LOG_DEBUG=all VK_LOADER_DEBUG=all`
Pipe the application output to a file (`2>&1 | tee app.log` at the end of the command)

Upload both xxxx_lsfg.log and app.log.

Open up a terminal and type `vulkaninfo -o vulkan.txt`, this will create a file called vulkan.txt. Upload it as well.
