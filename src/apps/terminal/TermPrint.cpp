/*
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files or portions
 * thereof (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice
 *    in the  binary, as well as this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided with
 *    the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <PrintJob.h>

#include "TermView.h"

BMessage *setup = NULL;

////////////////////////////////////////////////////////////////////////////
// DoPageSetUp ()
//
////////////////////////////////////////////////////////////////////////////
void
TermView::DoPageSetup() 
{ 
  BPrintJob job("Page Setup");  
  
  if (setup) { 
    job.SetSettings(setup); 
  } 
  
  if (job.ConfigPage() == B_OK) { 
    delete setup; 
    setup = job.Settings(); 
  } 
}
  
////////////////////////////////////////////////////////////////////////////
// DoPrint ()
//
////////////////////////////////////////////////////////////////////////////
void
TermView::DoPrint()
{ 
  status_t result = B_OK; 
  BPrintJob job("document's name"); 
  
  // If we have no setup message, we should call ConfigPage() 
  // You must use the same instance of the BPrintJob object 
  // (you can't call the previous "PageSetup()" function, since it 
  // creates its own BPrintJob object). 
  
  if (!setup) { 
    result = job.ConfigPage(); 
    if (result == B_OK) { 
      // Get the user Settings 
      setup = job.Settings(); 
    } 
  } 
  
  if (result == B_OK) { 
    // Setup the driver with user settings 
    job.SetSettings(setup); 
  
    result = job.ConfigJob(); 
    if (result == B_OK) { 
    // WARNING: here, setup CANNOT be NULL. 
      if (setup == NULL) { 
      // something's wrong, handle the error and bail out 
      } 
    delete setup; 
  
    // Get the user Settings 
    setup = job.Settings(); 
    
    // Now you have to calculate the number of pages 
    // (note: page are zero-based) 
    int32 firstPage = job.FirstPage(); 
    int32 lastPage = job.LastPage(); 
  
    // Verify the range is correct 
    // 0 ... LONG_MAX -> Print all the document 
    // n ... LONG_MAX -> Print from page n to the end 
    // n ... m -> Print from page n to page m 
  
    //  if (lastPage > your_document_last_page) 
    //    last_page = your_document_last_page; 
  
    int32 nbPages = lastPage - firstPage + 1; 
  
    // Verify the range is correct 
    if (nbPages <= 0) 
      return; // ERROR; 
  
    // Now you can print the page 
    job.BeginJob(); 
  
    // Print all pages 
    bool can_continue = job.CanContinue(); 
  
    for (int i=firstPage ; can_continue && i<=lastPage ; i++) { 
      // Draw all the needed views 
      //job.DrawView(this, this->Bounds(), pointOnPage); 
  
      // If the document have a lot of pages, you can update a BStatusBar, here 
      // or else what you want... 
      //update_status_bar(i-firstPage, nbPages); 
  
      // Spool the page 
      job.SpoolPage(); 
  
      // Cancel the job if needed. 
      // You can for exemple verify if the user pressed the ESC key 
      // or (SHIFT + '.')... 
      //if (user_has_canceled) { 
        // tell the print_server to cancel the printjob 
        //job.CancelJob(); 
        //can_continue = false; 
        //break; 
      //} 
  
      // Verify that there was no error (disk full for exemple) 
      can_continue = job.CanContinue(); 
    } 
  
    // Now, you just have to commit the job! 
    if (can_continue) 
      job.CommitJob(); 
    else 
      result = B_ERROR; 
    } 
  } 
  
}