/*=============================================================================
  Copyright (C) 2017 Allied Vision Technologies.  All Rights Reserved.

-------------------------------------------------------------------------------

  File:        Version.h

  Description: The Basic V4L2 Demo will demonstrate how to open and initialize
               a V4L2 device and how to acquire and display images from it.

-------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=============================================================================*/

#ifndef VERSION_H
#define VERSION_H

#define TOSTRING_HELPER(x) #x
//We need this additional preprocessor indirection because otherwise it won't work!
#define TOSTRING(x) TOSTRING_HELPER(x)

//Defines for naming and versioning of the application
#define APPLICATION_VERSION_MAJOR   1
#define APPLICATION_VERSION_MINOR   0
#define APPLICATION_VERSION_PATCH   0
#define APPLICATION_VERSION         (TOSTRING(APPLICATION_VERSION_MAJOR) "." TOSTRING(APPLICATION_VERSION_MINOR) "." TOSTRING(APPLICATION_VERSION_PATCH))
#define APPLICATION_PRODUCT_NAME    "Basic V4L2 Demo"
#define APPLICATION_FILE_NAME       "BasicDemo"
#define APPLICATION_COPYRIGHT       "(c) Allied Vision Technologies 2017"


#endif //VERSION_H 
