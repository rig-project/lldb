"""Test the SBData APIs."""

import os
import unittest2
import lldb
import pexpect
from lldbtest import *
from math import fabs
import lldbutil

class SBDataAPICase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @python_api_test
    @dsym_test
    def test_with_dsym_and_run_command(self):
        """Test the SBData APIs."""
        self.buildDsym()
        self.data_api()

    @python_api_test
    @dwarf_test
    def test_with_dwarf_and_run_command(self):
        """Test the SBData APIs."""
        self.buildDwarf()
        self.data_api()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break on inside main.cpp.
        self.line = line_number('main.cpp', '// set breakpoint here')

    def assert_data(self, func, arg, expected):
        """ Asserts func(SBError error, arg) == expected. """
        error = lldb.SBError()
        result = func(error, arg)
        if not error.Success():
            stream = lldb.SBStream()
            error.GetDescription(stream)
            self.assertTrue(error.Success(),
                            "%s(error, %s) did not succeed: %s" % (func.__name__,
                                                                   arg,
                                                                   stream.GetData()))
        self.assertTrue(expected == result, "%s(error, %s) == %s != %s" % (func.__name__, arg, result, expected))
          
    def data_api(self):
        """Test the SBData APIs."""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)
        
        lldbutil.run_break_set_by_file_and_line (self, "main.cpp", self.line, num_expected_locations=1, loc_exact=True)
        
        self.runCmd("run", RUN_SUCCEEDED)
        
        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
                    substrs = ['stopped',
                               'stop reason = breakpoint'])
        
        target = self.dbg.GetSelectedTarget()
        
        process = target.GetProcess()
        
        thread = process.GetThreadAtIndex(0)

        frame = thread.GetSelectedFrame()
        if self.TraceOn():
            print frame
        foobar = frame.FindVariable('foobar')
        self.assertTrue(foobar.IsValid())
        if self.TraceOn():
            print foobar

        data = foobar.GetPointeeData(0, 2)

        if self.TraceOn():
            print data

        offset = 0
        error = lldb.SBError()

        self.assert_data(data.GetUnsignedInt32, offset, 1)
        offset += 4
        low = data.GetSignedInt16(error, offset)
        self.assertTrue(error.Success())
        offset += 2
        high = data.GetSignedInt16(error, offset)
        self.assertTrue(error.Success())
        offset += 2
        self.assertTrue ((low == 9 and high == 0) or (low == 0 and high == 9), 'foo[0].b == 9')
        self.assertTrue( fabs(data.GetFloat(error, offset) - 3.14) < 1, 'foo[0].c == 3.14')
        self.assertTrue(error.Success())
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 8)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 5)
        offset += 4

        self.runCmd("n")

        offset = 16

        self.assert_data(data.GetUnsignedInt32, offset, 5)

        data = foobar.GetPointeeData(1, 1)

        offset = 0

        self.assert_data(data.GetSignedInt32, offset, 8)
        offset += 4
        self.assert_data(data.GetSignedInt32, offset, 7)
        offset += 8
        self.assertTrue(data.GetUnsignedInt32(error, offset) == 0, 'do not read beyond end')
        self.assertTrue(not error.Success())
        error.Clear() # clear the error for the next test

        star_foobar = foobar.Dereference()
        self.assertTrue(star_foobar.IsValid())
        
        data = star_foobar.GetData()

        if self.TraceOn():
            print data
        
        offset = 0
        self.assert_data(data.GetUnsignedInt32, offset, 1)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 9)

        foobar_addr = star_foobar.GetLoadAddress()
        foobar_addr += 12

        # http://llvm.org/bugs/show_bug.cgi?id=11579
        # lldb::SBValue::CreateValueFromAddress does not verify SBType::GetPointerType succeeds
        # This should not crash LLDB.
        nothing = foobar.CreateValueFromAddress("nothing", foobar_addr, star_foobar.GetType().GetBasicType(lldb.eBasicTypeInvalid))

        new_foobar = foobar.CreateValueFromAddress("f00", foobar_addr, star_foobar.GetType())
        self.assertTrue(new_foobar.IsValid())
        if self.TraceOn():
            print new_foobar
        
        data = new_foobar.GetData()

        if self.TraceOn():
            print data

        self.assertTrue(data.uint32[0] == 8, 'then foo[1].a == 8')
        self.assertTrue(data.uint32[1] == 7, 'then foo[1].b == 7')
        self.assertTrue(fabs(data.float[2] - 3.14) < 1, 'foo[1].c == 3.14') # exploiting that sizeof(uint32) == sizeof(float)

        self.runCmd("n")

        offset = 0
        self.assert_data(data.GetUnsignedInt32, offset, 8)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 7)
        offset += 4
        self.assertTrue(fabs(data.GetFloat(error, offset) - 3.14) < 1, 'foo[1].c == 3.14')
        self.assertTrue(error.Success())

        data = new_foobar.GetData()

        if self.TraceOn():
            print data

        offset = 0
        self.assert_data(data.GetUnsignedInt32, offset, 8)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 7)
        offset += 4
        self.assertTrue(fabs(data.GetFloat(error, offset) - 6.28) < 1, 'foo[1].c == 6.28')
        self.assertTrue(error.Success())

        self.runCmd("n")

        barfoo = frame.FindVariable('barfoo')

        data = barfoo.GetData()

        if self.TraceOn():
            print barfoo

        if self.TraceOn():
            print data

        offset = 0
        self.assert_data(data.GetUnsignedInt32, offset, 1)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 2)
        offset += 4
        self.assertTrue(fabs(data.GetFloat(error, offset) - 3) < 1, 'barfoo[0].c == 3')
        self.assertTrue(error.Success())
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 4)
        offset += 4
        self.assert_data(data.GetUnsignedInt32, offset, 5)
        offset += 4
        self.assertTrue(fabs(data.GetFloat(error, offset) - 6) < 1, 'barfoo[1].c == 6')
        self.assertTrue(error.Success())

        new_object = barfoo.CreateValueFromData("new_object",data,barfoo.GetType().GetBasicType(lldb.eBasicTypeInt))

        if self.TraceOn():
            print new_object
        
        self.assertTrue(new_object.GetLoadAddress() == 0xFFFFFFFFFFFFFFFF, 'GetLoadAddress() == invalid')
        self.assertTrue(new_object.AddressOf().IsValid() == False, 'AddressOf() == invalid')
        self.assertTrue(new_object.GetAddress().IsValid() == False, 'GetAddress() == invalid')

        self.assertTrue(new_object.GetValue() == "1", 'new_object == 1')

        data.SetData(error, 'A\0\0\0', data.GetByteOrder(), data.GetAddressByteSize())
        self.assertTrue(error.Success())
        
        data2 = lldb.SBData()
        data2.SetData(error, 'BCD', data.GetByteOrder(), data.GetAddressByteSize())
        self.assertTrue(error.Success())

        data.Append(data2)
        
        if self.TraceOn():
            print data

        # this breaks on EBCDIC
        offset = 0
        self.assert_data(data.GetUnsignedInt32, offset, 65)
        offset += 4
        self.assert_data(data.GetUnsignedInt8, offset, 66)
        offset += 1
        self.assert_data(data.GetUnsignedInt8, offset, 67)
        offset += 1
        self.assert_data(data.GetUnsignedInt8, offset, 68)
        offset += 1

        # check the new API calls introduced per LLVM llvm.org/prenhancement request
        # 11619 (Allow creating SBData values from arrays or primitives in Python)

        hello_str = "hello!"
        data2 = lldb.SBData.CreateDataFromCString(process.GetByteOrder(),process.GetAddressByteSize(),hello_str)
        self.assertTrue(len(data2.uint8) == len(hello_str))
        self.assertTrue(data2.uint8[0] == 104, 'h == 104')
        self.assertTrue(data2.uint8[1] == 101, 'e == 101')
        self.assertTrue(data2.uint8[2] == 108, 'l == 108')
        self.assert_data(data2.GetUnsignedInt8, 3, 108) # l
        self.assertTrue(data2.uint8[4] == 111, 'o == 111')
        self.assert_data(data2.GetUnsignedInt8, 5, 33) # !
        
        data2 = lldb.SBData.CreateDataFromUInt64Array(process.GetByteOrder(),process.GetAddressByteSize(),[1,2,3,4,5])
        self.assert_data(data2.GetUnsignedInt64, 0, 1)
        self.assert_data(data2.GetUnsignedInt64, 8, 2)
        self.assert_data(data2.GetUnsignedInt64, 16, 3)
        self.assert_data(data2.GetUnsignedInt64, 24, 4)
        self.assert_data(data2.GetUnsignedInt64, 32, 5)
        
        self.assertTrue(data2.uint64s == [1,2,3,4,5], 'read_data_helper failure: data2 == [1,2,3,4,5]')

        data2 = lldb.SBData.CreateDataFromSInt32Array(process.GetByteOrder(),process.GetAddressByteSize(),[2, -2])
        self.assertTrue(data2.sint32[0:2] == [2,-2], 'signed32 data2 = [2,-2]')
        
        data2.Append(lldb.SBData.CreateDataFromSInt64Array(process.GetByteOrder(),process.GetAddressByteSize(),[2, -2]))
        self.assert_data(data2.GetSignedInt32, 0, 2)
        self.assert_data(data2.GetSignedInt32, 4, -2)
        self.assertTrue(data2.sint64[1:3] == [2,-2], 'signed64 data2 = [2,-2]')
        
        data2 = lldb.SBData.CreateDataFromUInt32Array(process.GetByteOrder(),process.GetAddressByteSize(),[1,2,3,4,5])
        self.assert_data(data2.GetUnsignedInt32,0, 1)
        self.assert_data(data2.GetUnsignedInt32,4, 2)
        self.assert_data(data2.GetUnsignedInt32,8, 3)
        self.assert_data(data2.GetUnsignedInt32,12, 4)
        self.assert_data(data2.GetUnsignedInt32,16, 5)
        
        data2 = lldb.SBData.CreateDataFromDoubleArray(process.GetByteOrder(),process.GetAddressByteSize(),[3.14,6.28,2.71])
        self.assertTrue( fabs(data2.GetDouble(error,0) - 3.14) < 0.5, 'double data2[0] = 3.14')
        self.assertTrue(error.Success())
        self.assertTrue( fabs(data2.GetDouble(error,8) - 6.28) < 0.5, 'double data2[1] = 6.28')
        self.assertTrue(error.Success())
        self.assertTrue( fabs(data2.GetDouble(error,16) - 2.71) < 0.5, 'double data2[2] = 2.71')
        self.assertTrue(error.Success())

        data2 = lldb.SBData()

        data2.SetDataFromCString(hello_str)
        self.assertTrue(len(data2.uint8) == len(hello_str))
        self.assert_data(data2.GetUnsignedInt8, 0, 104)
        self.assert_data(data2.GetUnsignedInt8, 1, 101)
        self.assert_data(data2.GetUnsignedInt8, 2, 108)
        self.assert_data(data2.GetUnsignedInt8, 3, 108)
        self.assert_data(data2.GetUnsignedInt8, 4, 111)
        self.assert_data(data2.GetUnsignedInt8, 5, 33)

        data2.SetDataFromUInt64Array([1,2,3,4,5])
        self.assert_data(data2.GetUnsignedInt64, 0, 1)
        self.assert_data(data2.GetUnsignedInt64, 8,  2)
        self.assert_data(data2.GetUnsignedInt64, 16, 3)
        self.assert_data(data2.GetUnsignedInt64, 24, 4)
        self.assert_data(data2.GetUnsignedInt64, 32, 5)

        self.assertTrue(data2.uint64[0] == 1, 'read_data_helper failure: set data2[0] = 1')
        self.assertTrue(data2.uint64[1] == 2, 'read_data_helper failure: set data2[1] = 2')
        self.assertTrue(data2.uint64[2] == 3, 'read_data_helper failure: set data2[2] = 3')
        self.assertTrue(data2.uint64[3] == 4, 'read_data_helper failure: set data2[3] = 4')
        self.assertTrue(data2.uint64[4] == 5, 'read_data_helper failure: set data2[4] = 5')

        self.assertTrue(data2.uint64[0:2] == [1,2], 'read_data_helper failure: set data2[0:2] = [1,2]')

        data2.SetDataFromSInt32Array([2, -2])
        self.assert_data(data2.GetSignedInt32, 0, 2)
        self.assert_data(data2.GetSignedInt32, 4, -2)
        
        data2.SetDataFromSInt64Array([2, -2])
        self.assert_data(data2.GetSignedInt32, 0, 2)
        self.assert_data(data2.GetSignedInt32, 8, -2)
        
        data2.SetDataFromUInt32Array([1,2,3,4,5])
        self.assert_data(data2.GetUnsignedInt32, 0, 1)
        self.assert_data(data2.GetUnsignedInt32, 4, 2)
        self.assert_data(data2.GetUnsignedInt32, 8, 3)
        self.assert_data(data2.GetUnsignedInt32, 12, 4)
        self.assert_data(data2.GetUnsignedInt32, 16, 5)
        
        self.assertTrue(data2.uint32[0] == 1, 'read_data_helper failure: set 32-bit data2[0] = 1')
        self.assertTrue(data2.uint32[1] == 2, 'read_data_helper failure: set 32-bit data2[1] = 2')
        self.assertTrue(data2.uint32[2] == 3, 'read_data_helper failure: set 32-bit data2[2] = 3')
        self.assertTrue(data2.uint32[3] == 4, 'read_data_helper failure: set 32-bit data2[3] = 4')
        self.assertTrue(data2.uint32[4] == 5, 'read_data_helper failure: set 32-bit data2[4] = 5')

        data2.SetDataFromDoubleArray([3.14,6.28,2.71])
        self.assertTrue( fabs(data2.GetDouble(error,0) - 3.14) < 0.5, 'set double data2[0] = 3.14')
        self.assertTrue( fabs(data2.GetDouble(error,8) - 6.28) < 0.5, 'set double data2[1] = 6.28')
        self.assertTrue( fabs(data2.GetDouble(error,16) - 2.71) < 0.5, 'set double data2[2] = 2.71')

        self.assertTrue( fabs(data2.double[0] - 3.14) < 0.5, 'read_data_helper failure: set double data2[0] = 3.14')
        self.assertTrue( fabs(data2.double[1] - 6.28) < 0.5, 'read_data_helper failure: set double data2[1] = 6.28')
        self.assertTrue( fabs(data2.double[2] - 2.71) < 0.5, 'read_data_helper failure: set double data2[2] = 2.71')

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
