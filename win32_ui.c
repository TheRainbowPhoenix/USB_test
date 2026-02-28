#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#include "../fxlink.h"
#include <fxlink/filter.h>
#include <fxlink/logging.h>
#include <fxlink/protocol.h>
#include <fxlink/devices.h>
#include <stdio.h>
#include <stdlib.h>

static HWND hMainWnd;
static HWND hListBox;
static HWND hEdit;
static HWND hBtnSend;
static libusb_context *usb_ctx;
static struct fxlink_device_list devices;

// Context menu handles
static HMENU hContextMenu;

static void append_log(const char *str) {
    if (!hListBox) return;
    char *copy = strdup(str);
    char *token = strtok(copy, "\n");
    while(token) {
        SendMessageA(hListBox, LB_ADDSTRING, 0, (LPARAM)token);
        SendMessageA(hListBox, WM_VSCROLL, SB_BOTTOM, 0);
        token = strtok(NULL, "\n");
    }
    free(copy);
}

static void log_handler(int display_fmt, char const *str) {
    (void)display_fmt;
    append_log(str);
}

static void send_command_to_all(const char *app, const char *type, const char *payload, int payload_len) {
    for(int i = 0; i < devices.count; i++) {
        struct fxlink_device *fdev = &devices.devices[i];
        if(fdev->status == FXLINK_FDEV_STATUS_CONNECTED) {
            // fxlink_device_start_bulk_OUT takes ownership if last arg is true. We pass a malloc'd copy if needed.
            char *out = malloc(payload_len);
            memcpy(out, payload, payload_len);
            fxlink_device_start_bulk_OUT(fdev, app, type, out, payload_len, true);
        }
    }
}

static void execute_command(const char *cmd) {
    if(strncmp(cmd, "/echo ", 6) == 0) {
        const char *payload = cmd + 6;
        int len = strlen(payload);
        char *out = malloc(len + 6);
        sprintf(out, "echo %s\n", payload);
        send_command_to_all("fxlink", "command", out, strlen(out));
        free(out);
    } else if(strcmp(cmd, "/quit") == 0) {
        // Send quit command to python script
        const char *payload = "quit\n";
        send_command_to_all("fxlink", "command", payload, strlen(payload));
        PostQuitMessage(0); // Also exit PC UI
    } else if(strcmp(cmd, "/vram") == 0) {
        // Send vram command to python script
        const char *payload = "vram\n";
        send_command_to_all("fxlink", "command", payload, strlen(payload));
    } else if(strcmp(cmd, "/identify") == 0) {
        const char *payload = "identify\n";
        send_command_to_all("fxlink", "command", payload, strlen(payload));
    } else if(strcmp(cmd, "/screenshot") == 0) {
        const char *payload = "screenshot\n";
        send_command_to_all("fxlink", "command", payload, strlen(payload));
    } else if(strcmp(cmd, "/video") == 0) {
        const char *payload = "video\n";
        send_command_to_all("fxlink", "command", payload, strlen(payload));
    } else {
        // Send raw text as 'text' type for generic chat/data
        send_command_to_all("fxlink", "text", cmd, strlen(cmd));
        // Also log locally
        char buf[256];
        snprintf(buf, sizeof(buf), "[TX] %s", cmd);
        append_log(buf);
    }
}

static void copy_to_clipboard(HWND hwnd, const char* text) {
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE, strlen(text) + 1);
        if (hg) {
            memcpy(GlobalLock(hg), text, strlen(text) + 1);
            GlobalUnlock(hg);
            SetClipboardData(CF_TEXT, hg);
        }
        CloseClipboard();
    }
}

static void save_to_file(HWND hwnd, const char* text) {
    OPENFILENAMEA ofn;
    char szFileName[MAX_PATH] = "";

    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = "Text Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = "txt";

    if(GetSaveFileNameA(&ofn)) {
        FILE *fp = fopen(szFileName, "wb");
        if(fp) {
            fwrite(text, 1, strlen(text), fp);
            fclose(fp);
            append_log("Message saved to file.");
        } else {
            append_log("Failed to save file.");
        }
    }
}

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            hListBox = CreateWindowExA(WS_EX_CLIENTEDGE, "LISTBOX", "",
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOINTEGRALHEIGHT | LBS_NOTIFY,
                10, 10, 560, 300, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
            SendMessage(hListBox, WM_SETFONT, (WPARAM)hFont, TRUE);

            hEdit = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
                10, 320, 470, 25, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
            SendMessage(hEdit, WM_SETFONT, (WPARAM)hFont, TRUE);

            hBtnSend = CreateWindowExA(0, "BUTTON", "Send",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                490, 320, 80, 25, hwnd, (HMENU)3, GetModuleHandle(NULL), NULL);
            SendMessage(hBtnSend, WM_SETFONT, (WPARAM)hFont, TRUE);

            // Create context menu
            hContextMenu = CreatePopupMenu();
            AppendMenuA(hContextMenu, MF_STRING, 1001, "Copy Message");
            AppendMenuA(hContextMenu, MF_STRING, 1002, "Save to File");

            // Set a timer to poll libusb
            SetTimer(hwnd, 1, 50, NULL);
            return 0;
        }

        case WM_CONTEXTMENU: {
            if ((HWND)wParam == hListBox) {
                int sel = SendMessageA(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    POINT pt;
                    pt.x = LOWORD(lParam);
                    pt.y = HIWORD(lParam);
                    TrackPopupMenu(hContextMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
                }
            }
            return 0;
        }

        case WM_TIMER: {
            if (wParam == 1 && usb_ctx) {
                struct timeval zero_tv = { 0 };
                libusb_handle_events_timeout(usb_ctx, &zero_tv);
                fxlink_device_list_refresh(&devices);

                for(int i = 0; i < devices.count; i++) {
                    struct fxlink_device *fdev = &devices.devices[i];
                    if(fxlink_device_ready_to_connect(fdev) && fxlink_device_has_fxlink_interface(fdev)) {
                        if(fxlink_device_claim_fxlink(fdev)) {
                            append_log("Connected to calculator.");
                            fxlink_device_start_bulk_IN(fdev);
                        }
                    }

                    struct fxlink_message *msg;
                    while((msg = fxlink_device_finish_bulk_IN(fdev))) {
                        char buf[256];
                        sprintf(buf, "[RX] %.16s:%.16s (%d bytes)", msg->application, msg->type, msg->size);
                        append_log(buf);
                        if(strncmp(msg->application, "fxlink", 6) == 0 && strncmp(msg->type, "text", 4) == 0) {
                            char *txt = malloc(msg->size + 1);
                            memcpy(txt, msg->data, msg->size);
                            txt[msg->size] = 0;
                            append_log(txt);
                            free(txt);
                        } else if(strncmp(msg->application, "python", 6) == 0 && strncmp(msg->type, "text", 4) == 0) {
                            char *txt = malloc(msg->size + 1);
                            memcpy(txt, msg->data, msg->size);
                            txt[msg->size] = 0;
                            append_log(txt);
                            free(txt);
                        }
                        fxlink_message_free(msg, true);
                        fxlink_device_start_bulk_IN(fdev);
                    }
                }
            }
            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            int wmEvent = HIWORD(wParam);

            if (wmId == 3 || (wmId == 2 && wmEvent == EN_MAXTEXT)) {
                // Send button clicked or edit max (hacky enter)
                if (wmId == 3) {
                    char buf[512];
                    GetWindowTextA(hEdit, buf, 512);
                    if (strlen(buf) > 0) {
                        execute_command(buf);
                        SetWindowTextA(hEdit, "");
                    }
                }
            } else if (wmId == 1001 || wmId == 1002) {
                // Context menu actions
                int sel = SendMessageA(hListBox, LB_GETCURSEL, 0, 0);
                if (sel != LB_ERR) {
                    int len = SendMessageA(hListBox, LB_GETTEXTLEN, sel, 0);
                    char *text = malloc(len + 1);
                    SendMessageA(hListBox, LB_GETTEXT, sel, (LPARAM)text);

                    if (wmId == 1001) {
                        copy_to_clipboard(hwnd, text);
                    } else if (wmId == 1002) {
                        save_to_file(hwnd, text);
                    }
                    free(text);
                }
            }
            return 0;
        }

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            DestroyMenu(hContextMenu);
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main_win32_interactive(libusb_context *ctx) {
    usb_ctx = ctx;
    fxlink_log_set_handler(log_handler);
    fxlink_device_list_track(&devices, ctx);

    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char CLASS_NAME[] = "FxlinkWin32Class";

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassA(&wc);

    hMainWnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "fxlink - Windows UI",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 400,
        NULL, NULL, hInstance, NULL
    );

    if (hMainWnd == NULL) {
        return 0;
    }

    ShowWindow(hMainWnd, SW_SHOW);
    append_log("fxlink Windows UI started. Waiting for connection...");
    append_log("Commands: /echo <text>, /vram, /quit, /screenshot, /identify");
    append_log("Right-click a line to copy or save.");

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    fxlink_device_list_stop(&devices);
    fxlink_log_set_handler(NULL);
    return 0;
}
#endif
