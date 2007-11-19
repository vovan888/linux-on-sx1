Attribute VB_Name = "libini"
'***************************************************************************
'                         libini.bas -  Visual Basic access functions
'                            -------------------
'   begin                : Fri Apr 21 2000
'   copyright            : (C) 2000 by Simon White
'   email                : s_a_white@email.com
'***************************************************************************

'***************************************************************************
'*                                                                         *
'*   This program is free software; you can redistribute it and/or modify  *
'*   it under the terms of the GNU General Public License as published by  *
'*   the Free Software Foundation; either version 2 of the License, or     *
'*   (at your option) any later version.                                   *
'*                                                                         *
'***************************************************************************
Option Explicit

Private buffer As String
Private length As Long

' Returns 0 for error or an ini file descriptor for passing to
' the other functions.  Mode is:
'     "r" - existing file for read only
'     "w" - new or existing file
'     "a" - existing file
' Comment is the character used for comments in the file e.g. ";"
Declare Function ini_open Lib "libini" (ByVal name As String, _
    ByVal mode As String, ByVal comment As String) As Long

' Returns -1 on error.
Declare Function ini_close Lib "libini" (ByVal ini_fd As Long) As Long

' Saves changes to the INI file.
' Returns -1 on error.
Declare Function ini_flush Lib "libini" (ByVal ini_fd As Long) As Long

' Combines the contents of src and dst ini file.  Src keys will
' overwrite dst keys where appropriate. Returns -1 on error.
Declare Function ini_append Lib "libini" (ByVal dst_fd As Long, _
    ByVal src_fd As Long) As Long

' To create a new key call ini_locateHeading, ini_locateKey.  Although these
' calls fail, what it actually indicates is that these things don't exist.
' Check the return value from the write operation to see if the new key was
' created.

' All return -1 on error.
Declare Function ini_locateKey Lib "libini" _
    (ByVal ini_fd As Long, ByVal key As String) As Long
Declare Function ini_locateHeading Lib "libini" _
    (ByVal ini_fd As Long, ByVal heading As String) As Long

' To delete a heading use ini_locateHeading then ini_deleteHeading.
' To delete a key use ini_locateHeading, ini_locateKey then ini_deleteKey
' To delete everything including the INI file itself use ini_delete
' The above can be performed the same as Micrsoft by:
'     ini_locateHeading then ini_writeString, etc

' All return -1 on error.
Declare Function ini_delete Lib "libini" (ByVal ini_fd As Long) As Long
Declare Function ini_deleteKey Lib "libini" (ByVal ini_fd As Long) As Long
Declare Function ini_deleteHeading Lib "libini" _
    (ByVal ini_fd As Long) As Long

Private Declare Function ini_dataLength Lib "libini" _
    (ByVal ini_fd As Long) As Long

Private Declare Function ini_readStringFixed Lib "libini" _
    Alias "ini_readString" (ByVal ini_fd As Long, _
    ByVal str As String, ByVal size As Long) As Long

Declare Function ini_writeString Lib "libini" (ByVal ini_fd As Long, _
    ByVal sString As String) As Long

' All return -1 on error.  The string operations will return the number
' of characters read/written if an error did not occur.
Declare Function ini_readInt Lib "libini" (ByVal ini_fd As Long, _
    ByRef value As Integer) As Long
Declare Function ini_readLong Lib "libini" (ByVal ini_fd As Long, _
    ByRef value As Long) As Long
Declare Function ini_readDouble Lib "libini" (ByVal ini_fd As Long, _
    ByRef value As Double) As Long

Declare Function ini_writeInt Lib "libini" (ByVal ini_fd As Long, _
    ByVal value As Integer) As Long
Declare Function ini_writeLong Lib "libini" (ByVal ini_fd As Long, _
    ByVal value As Long) As Long
Declare Function ini_writeDouble Lib "libini" (ByVal ini_fd As Long, _
    ByVal value As Double) As Long

' Use this to indicate the character sperating multiple key elements.  Only
' use this for keys which require it.  Disable it again by passing in
' vbNullString as the delimiter e.g.:
' key = data1, data2, data3
' ini_locateKey  (fd, "key")
' ini_listDelims (fd, ","); Enable
' ... do reads here.
' ini_listDelims (fd, vbNullString); Disable

' Returns -1 on error, or the number of data elements found after applying
' the delimiters
Declare Function ini_ListLength Lib "libini" (ByVal ini_fd As Long) As Long

' All return -1 on error
' Sets the character(s) used to seperate data elements in the key
Declare Function ini_ListDelims Lib "libini" (ByVal ini_fd As Long, _
    ByVal delims As String) As Long
    
' Set index of the data element to get on next read.  This will auto incrment
' when a read is performed.  Doing a key_locate will automatically reset the index
' to 0 (first item).
Declare Function ini_ListIndex Lib "libini" (ByVal ini_fd As Long, _
    ByVal index As Long) As Long

' Returns -1 on error or the number of characters read
Function ini_readString(ByVal ini_fd As Long, ByRef str As String) As Long
    ' Create a fixed buffer to read string
    Dim l As Long
    Dim ret As Long
    l = ini_dataLength(ini_fd)
    If (l < 0) Then
        GoTo ini_readString_error
    End If

    l = l + 1: ' Reserve place for NULL (chr$(0))
    If (l > length) Then
        ' Increase buffer size
        buffer = String(l, Chr$(0))
        length = l
    End If

    ret = ini_readStringFixed(ini_fd, buffer, length)
    If (ret < 0) Then
        GoTo ini_readString_error
    End If

    ' Remove C String termination character (NULL)
    str = Left(buffer, ret)
    ini_readString = ret
Exit Function

ini_readString_error:
    ini_readString = -1
End Function
