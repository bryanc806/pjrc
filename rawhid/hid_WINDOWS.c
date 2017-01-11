/* Simple Raw HID functions for Windows - for use with Teensy RawHID example
 * http://www.pjrc.com/teensy/rawhid.html
 * Copyright (c) 2009 PJRC.COM, LLC
 *
 *  rawhid_open - open 1 or more devices
 *  rawhid_recv - receive a packet
 *  rawhid_send - send a packet
 *  rawhid_close - close a device
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above description, website URL and copyright notice and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Version 1.0: Initial Release
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <windows.h>
#include <setupapi.h>
//#include <shared/hidsdi.h>
//#include <shared/hidclass.h>
#include <api/hidsdi.h>
#include <ddk/hidclass.h>

#include "hid.h"


// a list of all opened HID devices, so the caller can
// simply refer to them by number
typedef struct hid_struct hid_t;
struct hid_struct {
	HANDLE handle;
	int open;
	struct hid_struct *prev;
	struct hid_struct *next;
};

struct	instance_info
{
	int vid, pid;
	hid_t *first_hid;
	hid_t *last_hid;
	HANDLE rx_event;
	HANDLE tx_event;
	CRITICAL_SECTION rx_mutex;
	CRITICAL_SECTION tx_mutex;
} instances[10] = {0};

// private functions, not intended to be used from outside this file
struct instance_info	*get_instance(int vid, int pid);
static void add_hid(struct instance_info	*, hid_t *h);
static hid_t * get_hid(struct instance_info	*, int num);
static void free_all_hid(struct instance_info	*);
static void hid_close(hid_t *hid);
void print_win32_err(void);

struct	instance_info	*get_instance(int vid, int pid)
{
	int i;
	for (i = 0; i < _countof(instances); i++)
	{
		if (vid == instances[i].vid && pid == instances[i].pid)
			return &instances[i];
	}
	return 0;
}

struct	instance_info	*add_instance(int vid, int pid)
{
	int i;
	for (i = 0; i < _countof(instances); i++)
	{
		if (instances[i].vid == 0 && instances[i].pid == 0)
		{
			memset(&instances[i], 0, sizeof(struct instance_info));
			instances[i].vid = vid;
			instances[i].pid = pid;
			return &instances[i];
		}
	}
	return 0;
}

//  rawhid_recv - receive a packet
//    Inputs:
//	num = device to receive from (zero based)
//	buf = buffer to receive packet
//	len = buffer's size
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes received, or -1 on error
//
int rawhid_recv(int vid, int pid, int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD n, r;
	struct	instance_info	*instance;

	if (sizeof(tmpbuf) < len + 1) return -1;

	instance = get_instance(vid, pid);
	if (!instance)
		return -1;
	hid = get_hid(instance, num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&instance->rx_mutex);
	ResetEvent(&instance->rx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = instance->rx_event;
	if (!ReadFile(hid->handle, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) goto return_error;
		r = WaitForSingleObject(instance->rx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&instance->rx_mutex);
	if (n <= 0) return -1;
	n--;
	if ((int)n > len) n = len;
	memcpy(buf, tmpbuf + 1, n);
	return n;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&instance->rx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&instance->rx_mutex);
	return -1;
}

//  rawhid_send - send a packet
//    Inputs:
//	num = device to transmit to (zero based)
//	buf = buffer containing packet to send
//	len = number of bytes to transmit
//	timeout = time to wait, in milliseconds
//    Output:
//	number of bytes sent, or -1 on error
//
int rawhid_send(int vid, int pid, int num, void *buf, int len, int timeout)
{
	hid_t *hid;
	unsigned char tmpbuf[516];
	OVERLAPPED ov;
	DWORD n, r;
	struct	instance_info	*instance;
	if (sizeof(tmpbuf) < len + 1) return -1;

	instance = get_instance(vid, pid);
	if (!instance)
		return -1;
	hid = get_hid(instance, num);
	if (!hid || !hid->open) return -1;
	EnterCriticalSection(&instance->tx_mutex);
	ResetEvent(&instance->tx_event);
	memset(&ov, 0, sizeof(ov));
	ov.hEvent = instance->tx_event;
	tmpbuf[0] = 0;
	memcpy(tmpbuf + 1, buf, len);
	if (!WriteFile(hid->handle, tmpbuf, len + 1, NULL, &ov)) {
		if (GetLastError() != ERROR_IO_PENDING) goto return_error;
		r = WaitForSingleObject(instance->tx_event, timeout);
		if (r == WAIT_TIMEOUT) goto return_timeout;
		if (r != WAIT_OBJECT_0) goto return_error;
	}
	if (!GetOverlappedResult(hid->handle, &ov, &n, FALSE)) goto return_error;
	LeaveCriticalSection(&instance->tx_mutex);
	if (n <= 0) return -1;
	return n - 1;
return_timeout:
	CancelIo(hid->handle);
	LeaveCriticalSection(&instance->tx_mutex);
	return 0;
return_error:
	print_win32_err();
	LeaveCriticalSection(&instance->tx_mutex);
	return -1;
}

//  rawhid_open - open 1 or more devices
//
//    Inputs:
//	max = maximum number of devices to open
//	vid = Vendor ID, or -1 if any
//	pid = Product ID, or -1 if any
//	usage_page = top level usage page, or -1 if any
//	usage = top level usage number, or -1 if any
//    Output:
//	actual number of devices opened
//
int rawhid_open(int max, int vid, int pid, int usage_page, int usage)
{
        GUID guid;
        HDEVINFO info;
        DWORD index=0, reqd_size;
        SP_DEVICE_INTERFACE_DATA iface;
        SP_DEVICE_INTERFACE_DETAIL_DATA *details;
        HIDD_ATTRIBUTES attrib;
        PHIDP_PREPARSED_DATA hid_data;
        HIDP_CAPS capabilities;
        HANDLE h;
        BOOL ret;
	struct	instance_info	*instance;
	hid_t *hid;
	int count=0;

	instance = get_instance(vid, pid);
	if (!instance)
	{
		instance = add_instance(vid, pid);
	}

	if (instance->first_hid) free_all_hid(instance);
	if (max < 1) return 0;
	if (!instance->rx_event) {
		instance->rx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		instance->tx_event = CreateEvent(NULL, TRUE, TRUE, NULL);
		InitializeCriticalSection(&instance->rx_mutex);
		InitializeCriticalSection(&instance->tx_mutex);
	}
	HidD_GetHidGuid(&guid);
	info = SetupDiGetClassDevs(&guid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
	if (info == INVALID_HANDLE_VALUE) return 0;
	for (index=0; 1 ;index++) {
		iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
		ret = SetupDiEnumDeviceInterfaces(info, NULL, &guid, index, &iface);
		if (!ret) return count;
		SetupDiGetInterfaceDeviceDetail(info, &iface, NULL, 0, &reqd_size, NULL);
		details = (SP_DEVICE_INTERFACE_DETAIL_DATA *)malloc(reqd_size);
		if (details == NULL) continue;

		memset(details, 0, reqd_size);
		details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
		ret = SetupDiGetDeviceInterfaceDetail(info, &iface, details,
			reqd_size, NULL, NULL);
		if (!ret) {
			free(details);
			continue;
		}
		h = CreateFile(details->DevicePath, GENERIC_READ|GENERIC_WRITE,
			FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
			OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
		free(details);
		if (h == INVALID_HANDLE_VALUE) continue;
		attrib.Size = sizeof(HIDD_ATTRIBUTES);
		ret = HidD_GetAttributes(h, &attrib);
		//printf("vid: %4x\n", attrib.VendorID);
		if (!ret || (vid > 0 && attrib.VendorID != vid) ||
		  (pid > 0 && attrib.ProductID != pid) ||
		  !HidD_GetPreparsedData(h, &hid_data)) {
			CloseHandle(h);
			continue;
		}
		if (!HidP_GetCaps(hid_data, &capabilities) ||
		  (usage_page > 0 && capabilities.UsagePage != usage_page) ||
		  (usage > 0 && capabilities.Usage != usage)) {
			HidD_FreePreparsedData(hid_data);
			CloseHandle(h);
			continue;
		}
		HidD_FreePreparsedData(hid_data);
		hid = (struct hid_struct *)malloc(sizeof(struct hid_struct));
		if (!hid) {
			CloseHandle(h);
			continue;
		}
		hid->handle = h;
		hid->open = 1;
		add_hid(instance, hid);
		count++;
		if (count >= max) return count;
	}
	return count;
}


//  rawhid_close - close a device
//
//    Inputs:
//	num = device to close (zero based)
//    Output
//	(nothing)
//
void rawhid_close(int vid, int pid, int num)
{
	hid_t *hid;
	struct	instance_info	*instance = get_instance(vid, pid);
	if (!instance)
		return;
	hid = get_hid(instance, num);
	if (!hid || !hid->open) return;
	hid_close(hid);
}



static void add_hid(struct instance_info *instance, hid_t *h)
{
	if (!instance->first_hid || !instance->last_hid) {
		instance->first_hid = instance->last_hid = h;
		h->next = h->prev = NULL;
		return;
	}
	instance->last_hid->next = h;
	h->prev = instance->last_hid;
	h->next = NULL;
	instance->last_hid = h;
}


static hid_t * get_hid(struct instance_info *instance, int num)
{
	hid_t *p;
	for (p = instance->first_hid; p && num > 0; p = p->next, num--) ;
	return p;
}


static void free_all_hid(struct instance_info	*instance)
{
	hid_t *p, *q;

	for (p = instance->first_hid; p; p = p->next) {
		hid_close(p);
	}
	p = instance->first_hid;
	while (p) {
		q = p;
		p = p->next;
		free(q);
	}
	instance->first_hid = instance->last_hid = NULL;
}


static void hid_close(hid_t *hid)
{
	CloseHandle(hid->handle);
	hid->handle = NULL;
}


void print_win32_err(void)
{
	char buf[256];
	DWORD err;

	err = GetLastError();
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, err,
		0, buf, sizeof(buf), NULL);
	printf("err %ld: %s\n", err, buf);
}





