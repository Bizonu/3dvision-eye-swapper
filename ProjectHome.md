# 3D Vision Eye Swapper #

_Inspired by the application made by Drejzik on [3D Vision Forum](http://3dvision-blog.com/forum/viewtopic.php?f=13&t=2206).
I've written this tool, because the original application did not always worked._


---

Some passive 3D monitors that are not certified by nVIDIA for 3D Vision require [EDID overriding](http://3dvision-blog.com/forum/viewtopic.php?f=14&t=2025) to enable Stereoscopic 3D in the nVIDIA control panel. Doing so, you may find out that the eyes are swapped and you require to wear the passive glasses upside down...

To avoid such awkward use of the glasses, this tool can be used to enable the swapping of the eyes from inside the 3D Vision driver.

What this tool does is very simple:
  1. Open the registry key **HKLM\SOFTWARE\Wow6432Node\NVIDIA Corporation\Global\Stereo3D** (for 64 bits Windows) or **HKLM\SOFTWARE\NVIDIA Corporation\Global\Stereo3D** (for 32 bits Windows).
  1. Write the value _0xFF00FF00_ (instead of _0x00FF00FF_) to the keys **InterleavePattern0** and **InterleavePattern1**.
  1. Monitor these keys and when they get changed by 3D Vision service, quickly write back the _0xFF00FF00_ value.

Note that this tool requires administrator rights to be able to change the registry keys...

## [Download 3D Vision Eye Swapper v1.0](https://3dvision-eye-swapper.googlecode.com/svn/tags/3DVisionEyeSwapper_v1.0.0.0/bin/Win32/Release/3DVisionEyeSwapper.exe) ##