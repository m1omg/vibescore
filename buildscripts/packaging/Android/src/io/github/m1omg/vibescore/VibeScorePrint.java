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
package io.github.m1omg.vibescore;

import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.os.CancellationSignal;
import android.os.ParcelFileDescriptor;
import android.print.PageRange;
import android.print.PrintAttributes;
import android.print.PrintDocumentAdapter;
import android.print.PrintDocumentInfo;
import android.print.PrintManager;
import android.util.Log;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Prints a PDF file with the system print dialog (printers of the device, "Save as PDF").
 * Called from native code.
 */
public class VibeScorePrint
{
    private static final String TAG = "VibeScore";

    public static void printPdf(final Context context, final String pdfPath, final String jobName)
    {
        if (!(context instanceof Activity)) {
            Log.e(TAG, "Printing needs an activity");
            return;
        }

        final Activity activity = (Activity) context;
        activity.runOnUiThread(() -> {
            try {
                PrintManager printManager = (PrintManager) activity.getSystemService(Context.PRINT_SERVICE);
                printManager.print(jobName, new PdfDocumentAdapter(pdfPath, jobName), null);
            } catch (Exception e) {
                Log.e(TAG, "Failed to print: " + e);
            }
        });
    }

    private static class PdfDocumentAdapter extends PrintDocumentAdapter
    {
        private final String m_pdfPath;
        private final String m_jobName;

        PdfDocumentAdapter(String pdfPath, String jobName)
        {
            m_pdfPath = pdfPath;
            m_jobName = jobName;
        }

        @Override
        public void onLayout(PrintAttributes oldAttributes, PrintAttributes newAttributes,
                             CancellationSignal cancellationSignal, LayoutResultCallback callback, Bundle extras)
        {
            if (cancellationSignal.isCanceled()) {
                callback.onLayoutCancelled();
                return;
            }

            // The pages are already laid out by the score's page settings
            PrintDocumentInfo info = new PrintDocumentInfo.Builder(m_jobName + ".pdf")
                .setContentType(PrintDocumentInfo.CONTENT_TYPE_DOCUMENT)
                .setPageCount(PrintDocumentInfo.PAGE_COUNT_UNKNOWN)
                .build();

            callback.onLayoutFinished(info, !newAttributes.equals(oldAttributes));
        }

        @Override
        public void onWrite(PageRange[] pages, ParcelFileDescriptor destination,
                            CancellationSignal cancellationSignal, WriteResultCallback callback)
        {
            try (InputStream in = new FileInputStream(m_pdfPath);
                 OutputStream out = new FileOutputStream(destination.getFileDescriptor())) {
                byte[] buffer = new byte[64 * 1024];
                int n;
                while ((n = in.read(buffer)) > 0) {
                    if (cancellationSignal.isCanceled()) {
                        callback.onWriteCancelled();
                        return;
                    }
                    out.write(buffer, 0, n);
                }
                callback.onWriteFinished(new PageRange[] { PageRange.ALL_PAGES });
            } catch (Exception e) {
                Log.e(TAG, "Failed to write the document to print: " + e);
                callback.onWriteFailed(e.toString());
            }
        }
    }
}
