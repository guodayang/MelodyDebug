// Copyright (c) 2012 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool. If making changes by
// hand only do so within the body of existing method and function
// implementations. See the translator.README.txt file in the tools directory
// for more information.
//

#include "libcef_dll/cpptoc/browser_cpptoc.h"
#include "libcef_dll/cpptoc/cookie_manager_cpptoc.h"
#include "libcef_dll/cpptoc/frame_cpptoc.h"
#include "libcef_dll/cpptoc/request_cpptoc.h"
#include "libcef_dll/cpptoc/response_cpptoc.h"
#include "libcef_dll/cpptoc/stream_reader_cpptoc.h"
#include "libcef_dll/ctocpp/content_filter_ctocpp.h"
#include "libcef_dll/ctocpp/download_handler_ctocpp.h"
#include "libcef_dll/ctocpp/request_handler_ctocpp.h"


// VIRTUAL METHODS - Body may be edited by hand.

bool CefRequestHandlerCToCpp::OnBeforeBrowse(CefRefPtr<CefBrowser> browser,
    CefRefPtr<CefFrame> frame, CefRefPtr<CefRequest> request, NavType navType,
    bool isRedirect) {
  if (CEF_MEMBER_MISSING(struct_, on_before_browse))
    return false;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return false;
  // Verify param: frame; type: refptr_diff
  DCHECK(frame.get());
  if (!frame.get())
    return false;
  // Verify param: request; type: refptr_diff
  DCHECK(request.get());
  if (!request.get())
    return false;

  // Execute
  int _retval = struct_->on_before_browse(struct_,
      CefBrowserCppToC::Wrap(browser),
      CefFrameCppToC::Wrap(frame),
      CefRequestCppToC::Wrap(request),
      navType,
      isRedirect);

  // Return type: bool
  return _retval?true:false;
}

bool CefRequestHandlerCToCpp::OnBeforeResourceLoad(
    CefRefPtr<CefBrowser> browser, CefRefPtr<CefRequest> request,
    CefString& redirectUrl, CefRefPtr<CefStreamReader>& resourceStream,
    CefRefPtr<CefResponse> response, int loadFlags) {
  if (CEF_MEMBER_MISSING(struct_, on_before_resource_load))
    return false;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return false;
  // Verify param: request; type: refptr_diff
  DCHECK(request.get());
  if (!request.get())
    return false;
  // Verify param: response; type: refptr_diff
  DCHECK(response.get());
  if (!response.get())
    return false;

  // Translate param: resourceStream; type: refptr_diff_byref
  cef_stream_reader_t* resourceStreamStruct = NULL;
  if (resourceStream.get())
    resourceStreamStruct = CefStreamReaderCppToC::Wrap(resourceStream);
  cef_stream_reader_t* resourceStreamOrig = resourceStreamStruct;

  // Execute
  int _retval = struct_->on_before_resource_load(struct_,
      CefBrowserCppToC::Wrap(browser),
      CefRequestCppToC::Wrap(request),
      redirectUrl.GetWritableStruct(),
      &resourceStreamStruct,
      CefResponseCppToC::Wrap(response),
      loadFlags);

  // Restore param:resourceStream; type: refptr_diff_byref
  if (resourceStreamStruct) {
    if (resourceStreamStruct != resourceStreamOrig) {
      resourceStream = CefStreamReaderCppToC::Unwrap(resourceStreamStruct);
    }
  } else {
    resourceStream = NULL;
  }

  // Return type: bool
  return _retval?true:false;
}

void CefRequestHandlerCToCpp::OnResourceRedirect(CefRefPtr<CefBrowser> browser,
    const CefString& old_url, CefString& new_url) {
  if (CEF_MEMBER_MISSING(struct_, on_resource_redirect))
    return;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return;
  // Verify param: old_url; type: string_byref_const
  DCHECK(!old_url.empty());
  if (old_url.empty())
    return;

  // Execute
  struct_->on_resource_redirect(struct_,
      CefBrowserCppToC::Wrap(browser),
      old_url.GetStruct(),
      new_url.GetWritableStruct());
}

void CefRequestHandlerCToCpp::OnResourceResponse(CefRefPtr<CefBrowser> browser,
    const CefString& url, CefRefPtr<CefResponse> response,
    CefRefPtr<CefContentFilter>& filter) {
  if (CEF_MEMBER_MISSING(struct_, on_resource_response))
    return;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return;
  // Verify param: url; type: string_byref_const
  DCHECK(!url.empty());
  if (url.empty())
    return;
  // Verify param: response; type: refptr_diff
  DCHECK(response.get());
  if (!response.get())
    return;

  // Translate param: filter; type: refptr_same_byref
  cef_content_filter_t* filterStruct = NULL;
  if (filter.get())
    filterStruct = CefContentFilterCToCpp::Unwrap(filter);
  cef_content_filter_t* filterOrig = filterStruct;

  // Execute
  struct_->on_resource_response(struct_,
      CefBrowserCppToC::Wrap(browser),
      url.GetStruct(),
      CefResponseCppToC::Wrap(response),
      &filterStruct);

  // Restore param:filter; type: refptr_same_byref
  if (filterStruct) {
    if (filterStruct != filterOrig) {
      filter = CefContentFilterCToCpp::Wrap(filterStruct);
    }
  } else {
    filter = NULL;
  }
}

bool CefRequestHandlerCToCpp::OnProtocolExecution(CefRefPtr<CefBrowser> browser,
    const CefString& url, bool& allowOSExecution) {
  if (CEF_MEMBER_MISSING(struct_, on_protocol_execution))
    return false;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return false;
  // Verify param: url; type: string_byref_const
  DCHECK(!url.empty());
  if (url.empty())
    return false;

  // Translate param: allowOSExecution; type: bool_byref
  int allowOSExecutionInt = allowOSExecution;

  // Execute
  int _retval = struct_->on_protocol_execution(struct_,
      CefBrowserCppToC::Wrap(browser),
      url.GetStruct(),
      &allowOSExecutionInt);

  // Restore param:allowOSExecution; type: bool_byref
  allowOSExecution = allowOSExecutionInt?true:false;

  // Return type: bool
  return _retval?true:false;
}

bool CefRequestHandlerCToCpp::GetDownloadHandler(CefRefPtr<CefBrowser> browser,
    const CefString& mimeType, const CefString& fileName, int64 contentLength,
    CefRefPtr<CefDownloadHandler>& handler) {
  if (CEF_MEMBER_MISSING(struct_, get_download_handler))
    return false;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return false;
  // Verify param: mimeType; type: string_byref_const
  DCHECK(!mimeType.empty());
  if (mimeType.empty())
    return false;
  // Verify param: fileName; type: string_byref_const
  DCHECK(!fileName.empty());
  if (fileName.empty())
    return false;

  // Translate param: handler; type: refptr_same_byref
  cef_download_handler_t* handlerStruct = NULL;
  if (handler.get())
    handlerStruct = CefDownloadHandlerCToCpp::Unwrap(handler);
  cef_download_handler_t* handlerOrig = handlerStruct;

  // Execute
  int _retval = struct_->get_download_handler(struct_,
      CefBrowserCppToC::Wrap(browser),
      mimeType.GetStruct(),
      fileName.GetStruct(),
      contentLength,
      &handlerStruct);

  // Restore param:handler; type: refptr_same_byref
  if (handlerStruct) {
    if (handlerStruct != handlerOrig) {
      handler = CefDownloadHandlerCToCpp::Wrap(handlerStruct);
    }
  } else {
    handler = NULL;
  }

  // Return type: bool
  return _retval?true:false;
}

bool CefRequestHandlerCToCpp::GetAuthCredentials(CefRefPtr<CefBrowser> browser,
    bool isProxy, const CefString& host, int port, const CefString& realm,
    const CefString& scheme, CefString& username, CefString& password) {
  if (CEF_MEMBER_MISSING(struct_, get_auth_credentials))
    return false;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return false;
  // Verify param: host; type: string_byref_const
  DCHECK(!host.empty());
  if (host.empty())
    return false;
  // Verify param: scheme; type: string_byref_const
  DCHECK(!scheme.empty());
  if (scheme.empty())
    return false;
  // Unverified params: realm

  // Execute
  int _retval = struct_->get_auth_credentials(struct_,
      CefBrowserCppToC::Wrap(browser),
      isProxy,
      host.GetStruct(),
      port,
      realm.GetStruct(),
      scheme.GetStruct(),
      username.GetWritableStruct(),
      password.GetWritableStruct());

  // Return type: bool
  return _retval?true:false;
}

CefRefPtr<CefCookieManager> CefRequestHandlerCToCpp::GetCookieManager(
    CefRefPtr<CefBrowser> browser, const CefString& main_url) {
  if (CEF_MEMBER_MISSING(struct_, get_cookie_manager))
    return NULL;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: browser; type: refptr_diff
  DCHECK(browser.get());
  if (!browser.get())
    return NULL;
  // Verify param: main_url; type: string_byref_const
  DCHECK(!main_url.empty());
  if (main_url.empty())
    return NULL;

  // Execute
  cef_cookie_manager_t* _retval = struct_->get_cookie_manager(struct_,
      CefBrowserCppToC::Wrap(browser),
      main_url.GetStruct());

  // Return type: refptr_diff
  return CefCookieManagerCppToC::Unwrap(_retval);
}

void CefRequestHandlerCToCpp::OnHttpResponse(const CefString& url,
    const CefString& mimeType, int responseCode, int64 contentLength,
    int64 startTime, int64 endTime) {
  if (CEF_MEMBER_MISSING(struct_, on_http_response))
    return;

  // AUTO-GENERATED CONTENT - DELETE THIS COMMENT BEFORE MODIFYING

  // Verify param: url; type: string_byref_const
  DCHECK(!url.empty());
  if (url.empty())
    return;
  // Unverified params: mimeType

  // Execute
  struct_->on_http_response(struct_,
      url.GetStruct(),
      mimeType.GetStruct(),
      responseCode,
      contentLength,
      startTime,
      endTime);
}


#ifndef NDEBUG
template<> long CefCToCpp<CefRequestHandlerCToCpp, CefRequestHandler,
    cef_request_handler_t>::DebugObjCt = 0;
#endif

