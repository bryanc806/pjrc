Teensy examples
http://www.pjrc.com/teensy
Copyright (c) 2009 PJRC.COM, LLC

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above description, website URL and copyright notice and this permission
notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

------------------------------------------------------------------------------

This git repo contains a collection of the teensy bare metal "Code Library"
snippets that can be downloaded one by one from their page. The purpose is easy
integration into other projects e.g. as git submodule, while keeping the
original code in one place.

Updates in the bryanc806 repo:

hid_WINDOWS.c has been updated (rather hackishly I should add) to
support multiple RawHID devices with different VID/PIDs and accessed
from different threads of the same program.

The API has changed a bit in that you provide the VID/PID on all
rawhid_recv() and rawhid_send() calls.

