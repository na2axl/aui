﻿#if defined(_WIN32)
#include <windows.h>
#include "AMessageBox.h"
#include "AWindow.h"
AMessageBox::Button AMessageBox::show(AWindow* parent, const AString& title, const AString& message, AMessageBox::Icon icon, AMessageBox::Button b)
{
    HWND window = parent ? parent->getNativeHandle() : nullptr;

    long flags = 0;

    // Icons
    if (icon & I_INFO)
    {
        flags |= MB_ICONINFORMATION;
    }
    if (icon & I_WARNING)
    {
        flags |= MB_ICONWARNING;
    }
    if (icon & I_CRITICAL)
    {
        flags |= MB_ICONSTOP;
    }

    // Flags
    if (b & B_OK)
    {
        flags |= MB_OK;
    }
    if (b & B_CANCEL)
    {
        flags |= MB_OKCANCEL;
    }

    switch (::MessageBox(window, message.c_str(), title.c_str(), flags)) {
    case IDOK:
        return B_OK;
    }
    return B_INVALID;
}
#elif defined(ANDROID)

#include <AUI/Platform/OSAndroid.h>
#include "AMessageBox.h"
#include "AWindow.h"

AMessageBox::Button
AMessageBox::show(AWindow *parent, const AString &title, const AString &message, AMessageBox::Icon icon,
                  AMessageBox::Button b) {

    auto j = AAndroid::getJNI();
    auto klazzAUI = j->FindClass("ru/alex2772/aui/AUI");
    auto methodShowMessageBox = j->GetStaticMethodID(klazzAUI, "showMessageBox", "(Ljava/lang/String;Ljava/lang/String;)V");
    auto strTitle = j->NewStringUTF(title.toStdString().c_str());
    auto strMessage = j->NewStringUTF(message.toStdString().c_str());

    j->CallStaticVoidMethod(klazzAUI, methodShowMessageBox, strTitle, strMessage);

    j->DeleteLocalRef(strTitle);
    j->DeleteLocalRef(strMessage);

    return B_INVALID;
}
#else
#include <gtk/gtk.h>
#include <AUI/Util/kAUI.h>
#include "AMessageBox.h"
#include "AWindow.h"
#include <AUI/Util/kAUI.h>

AMessageBox::Button
AMessageBox::show(AWindow *parent, const AString &title, const AString &message, AMessageBox::Icon icon,
                  AMessageBox::Button b) {
    unsigned iconFlags = 0;

    do_once {
        int argc = 0;
        char c = 0;
        char* pc = &c;
        char** ppc = &pc;

        gtk_init(&argc, &ppc);
    };


    // Icons
    switch (icon) {
        case I_INFO:
            iconFlags |= GTK_MESSAGE_INFO;
            break;
        case I_WARNING:
            iconFlags |= GTK_MESSAGE_WARNING;
            break;
        case I_CRITICAL:
            iconFlags |= GTK_MESSAGE_ERROR;
            break;
}

    unsigned buttonFlags = 0;

    // Flags
    if (b & B_OK) {
        buttonFlags |= GTK_BUTTONS_OK;
    }
    if (b & B_CANCEL) {
        buttonFlags |= GTK_BUTTONS_CANCEL;
    }

    auto dialog = gtk_message_dialog_new(nullptr,
                                         GTK_DIALOG_MODAL,
                                         (GtkMessageType)iconFlags,
                                         (GtkButtonsType)buttonFlags,
                                         "%ls", message.c_str());
    gtk_window_set_keep_above(GTK_WINDOW(dialog), true);
    gtk_window_activate_focus(GTK_WINDOW(dialog));
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(GTK_WIDGET(dialog));

    while (gtk_events_pending()) {
        gtk_main_iteration();
    }
    return B_INVALID;
}

#endif