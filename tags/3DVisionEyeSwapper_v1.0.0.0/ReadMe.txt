------------------------------------------------------------------------
                     nVIDIA 3D Vision Eye Swapper
------------------------------------------------------------------------

Inspired by the application made by Drejzik (http://3dvision-blog.com/forum/viewtopic.php?f=13&t=2206).
I've written this tools, because the original application did not always worked.


Some passive 3D monitors that are not certified by nVIDIA for 3D Vision require EDID overriding
(http://3dvision-blog.com/forum/viewtopic.php?f=14&t=2025) to enable Stereoscopic 3D in the
nVIDIA control panel. Doing so, you may find out that the eyes are swapped and you require to wear
the passive glasses uside down...

To avoid such awkward use of the glasses, this tool can be used to enable the swapping of the eyes
from inside the 3D Vision driver.



What this tool does is very simple:
    1. Open the registry key 'HKLM\SOFTWARE\Wow6432Node\NVIDIA Corporation\Global\Stereo3D' (for 64 bits Windows)
       or 'HKLM\SOFTWARE\NVIDIA Corporation\Global\Stereo3D' (for 32 bits Windows).
    2. Write the value 0xFF00FF00 (instead of 0x00FF00FF) to the keys InterleavePattern0 and InterleavePattern1.
    3. Monitor these keys and when they get changed by 3D Vision service, quickly write back the 0xFF00FF00 value.

Note that this tool requires administrator rights to be able to change the registry keys...
