/*
 * SPDX-License-Identifier: GPL-3.0-only
 * VibeScore - MuseScore Studio for mobile devices
 *
 * Copyright (C) 2026 VibeScore contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "iosplatform.h"

#import <UIKit/UIKit.h>
#import <CoreAudioKit/CoreAudioKit.h>

#include "log.h"

static UIViewController* topViewController()
{
    UIWindow* keyWindow = nil;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:UIWindowScene.class]) {
            continue;
        }

        for (UIWindow* window in static_cast<UIWindowScene*>(scene).windows) {
            if (window.isKeyWindow) {
                keyWindow = window;
                break;
            }
        }
    }

    UIViewController* controller = keyWindow.rootViewController;
    while (controller.presentedViewController) {
        controller = controller.presentedViewController;
    }

    return controller;
}

//! On iPad, sheets are popovers and need an anchor: the middle of the window
static void anchorPopover(UIPopoverPresentationController* popover, UIViewController* presenter)
{
    if (!popover) {
        return;
    }

    UIView* view = presenter.view;
    popover.sourceView = view;
    popover.sourceRect = CGRectMake(CGRectGetMidX(view.bounds), CGRectGetMidY(view.bounds), 1, 1);
    popover.permittedArrowDirections = 0;
}

void mu::mobile::ios::shareFiles(const QStringList& paths)
{
    NSMutableArray* items = [NSMutableArray arrayWithCapacity:paths.size()];
    for (const QString& path : paths) {
        [items addObject:[NSURL fileURLWithPath:path.toNSString()]];
    }

    UIViewController* presenter = topViewController();
    if (!presenter || items.count == 0) {
        LOGE() << "Can not share " << paths.join(", ");
        return;
    }

    UIActivityViewController* controller = [[UIActivityViewController alloc] initWithActivityItems:items applicationActivities:nil];
    anchorPopover(controller.popoverPresentationController, presenter);
    [presenter presentViewController:controller animated:YES completion:nil];
}

void mu::mobile::ios::printPdf(const QString& pdfPath, const QString& jobName)
{
    NSURL* url = [NSURL fileURLWithPath:pdfPath.toNSString()];
    if (![UIPrintInteractionController canPrintURL:url]) {
        LOGE() << "Can not print " << pdfPath;
        return;
    }

    UIPrintInfo* info = [UIPrintInfo printInfo];
    info.outputType = UIPrintInfoOutputGeneral;
    info.jobName = jobName.toNSString();

    UIPrintInteractionController* controller = UIPrintInteractionController.sharedPrintController;
    controller.printInfo = info;
    controller.printingItem = url;

    UIViewController* presenter = topViewController();
    if (UIDevice.currentDevice.userInterfaceIdiom == UIUserInterfaceIdiomPad && presenter) {
        UIView* view = presenter.view;
        const CGRect anchor = CGRectMake(CGRectGetMidX(view.bounds), CGRectGetMidY(view.bounds), 1, 1);
        [controller presentFromRect:anchor inView:view animated:YES completionHandler:nil];
    } else {
        [controller presentAnimated:YES completionHandler:nil];
    }
}

void mu::mobile::ios::showBluetoothMidiDevices()
{
    UIViewController* presenter = topViewController();
    if (!presenter) {
        LOGE() << "Can not show the Bluetooth MIDI devices";
        return;
    }

    // This file is compiled without ARC
    CABTMIDICentralViewController* devices = [[[CABTMIDICentralViewController alloc] init] autorelease];
    UINavigationController* navigation = [[[UINavigationController alloc] initWithRootViewController:devices] autorelease];

    // Done dismisses what the presenter presents (the block does not retain the navigation controller: no cycle)
    UIAction* done = [UIAction actionWithHandler:^(UIAction*) {
        [presenter dismissViewControllerAnimated:YES completion:nil];
    }];
    devices.navigationItem.rightBarButtonItem
        = [[[UIBarButtonItem alloc] initWithBarButtonSystemItem:UIBarButtonSystemItemDone primaryAction:done] autorelease];

    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [presenter presentViewController:navigation animated:YES completion:nil];
}
