This utility program is intended to simplify backup copies of any kind from one point to another. 
It is written in C and C++ to compile in Qt Creator with Qt6 platform. So it can be used on different enviroments, Microsoft Windows,
 Apple Mac or Linux platforms.
Copying can be done to compressed files using LZMA algorithm written by Igor Pavlov. Those files can also be recovered with 7z.exe.
The compressed files make it easy to save many versions of a same file so you can recover older versions if needed.

Th code for LZMA encoding and decoding comes from de LZMA SDK by Igor Pavlov. 

Many options are avalable, include and exclude subfolders, include and exclude files, type of copy, different comparasions of time stamp.
Form compresed copy files, you can use incremental system and choose the number of increments and the number of copies.
Operations can be simulated to see what would be done.
You can create many different operations, and choose those to be operated or not.
