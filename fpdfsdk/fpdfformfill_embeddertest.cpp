// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <memory>
#include <string>
#include <vector>

#include "core/fxcrt/fx_string.h"
#include "core/fxcrt/fx_system.h"
#include "public/cpp/fpdf_deleters.h"
#include "public/fpdf_formfill.h"
#include "public/fpdf_fwlevent.h"
#include "testing/embedder_test.h"
#include "testing/embedder_test_mock_delegate.h"
#include "testing/embedder_test_timer_handling_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using testing::_;
using testing::Return;

class FPDFFormFillEmbeddertest : public EmbedderTest, public TestSaver {
 protected:
  void TypeTextIntoTextfield(FPDF_PAGE page, int num_chars) {
    // Click on the textfield in text_form.pdf.
    EXPECT_EQ(FPDF_FORMFIELD_TEXTFIELD,
              FPDFPage_HasFormFieldAtPoint(form_handle(), page, 120.0, 120.0));
    FORM_OnMouseMove(form_handle(), page, 0, 120.0, 120.0);
    FORM_OnLButtonDown(form_handle(), page, 0, 120.0, 120.0);
    FORM_OnLButtonUp(form_handle(), page, 0, 120.0, 120.0);

    // Type text starting with 'A' to as many chars as specified by |num_chars|.
    for (int i = 0; i < num_chars; ++i) {
      FORM_OnChar(form_handle(), page, 'A' + i, 0);
    }
  }

  // Navigates to form text field using the mouse and then selects text via the
  // shift and specfied left or right arrow key.
  void SelectTextWithKeyboard(FPDF_PAGE page,
                              int num_chars,
                              int arrow_key,
                              double x,
                              double y) {
    // Navigate to starting position for selection.
    FORM_OnMouseMove(form_handle(), page, 0, x, y);
    FORM_OnLButtonDown(form_handle(), page, 0, x, y);
    FORM_OnLButtonUp(form_handle(), page, 0, x, y);

    // Hold down shift (and don't release until entire text is selected).
    FORM_OnKeyDown(form_handle(), page, FWL_VKEY_Shift, 0);

    // Select text char by char via left or right arrow key.
    for (int i = 0; i < num_chars; ++i) {
      FORM_OnKeyDown(form_handle(), page, arrow_key, FWL_EVENTFLAG_ShiftKey);
      FORM_OnKeyUp(form_handle(), page, arrow_key, FWL_EVENTFLAG_ShiftKey);
    }
    FORM_OnKeyUp(form_handle(), page, FWL_VKEY_Shift, 0);
  }

  // Uses the mouse to navigate to form text field and select text.
  void SelectTextWithMouse(FPDF_PAGE page,
                           double start_x,
                           double end_x,
                           double y) {
    // Navigate to starting position and click mouse.
    FORM_OnMouseMove(form_handle(), page, 0, start_x, y);
    FORM_OnLButtonDown(form_handle(), page, 0, start_x, y);

    // Hold down mouse until reach end of desired selection.
    FORM_OnMouseMove(form_handle(), page, 0, end_x, y);
    FORM_OnLButtonUp(form_handle(), page, 0, end_x, y);
  }

  void CheckSelection(FPDF_PAGE page, const CFX_WideString& expected_string) {
    // Calculate expected length for selected text.
    int num_chars = expected_string.GetLength();

    // Check actual selection against expected selection.
    const unsigned long expected_length =
        sizeof(unsigned short) * (num_chars + 1);
    unsigned long sel_text_len =
        FORM_GetSelectedText(form_handle(), page, nullptr, 0);
    ASSERT_EQ(expected_length, sel_text_len);

    std::vector<unsigned short> buf(sel_text_len);
    EXPECT_EQ(expected_length, FORM_GetSelectedText(form_handle(), page,
                                                    buf.data(), sel_text_len));

    EXPECT_EQ(expected_string,
              CFX_WideString::FromUTF16LE(buf.data(), num_chars));
  }
};

TEST_F(FPDFFormFillEmbeddertest, FirstTest) {
  EmbedderTestMockDelegate mock;
  EXPECT_CALL(mock, Alert(_, _, _, _)).Times(0);
  EXPECT_CALL(mock, UnsupportedHandler(_)).Times(0);
  EXPECT_CALL(mock, SetTimer(_, _)).Times(0);
  EXPECT_CALL(mock, KillTimer(_)).Times(0);
  SetDelegate(&mock);

  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, BUG_487928) {
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_487928.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  DoOpenActions();
  delegate.AdvanceTime(5000);
  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, BUG_507316) {
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_507316.pdf"));
  FPDF_PAGE page = LoadPage(2);
  EXPECT_TRUE(page);
  DoOpenActions();
  delegate.AdvanceTime(4000);
  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, BUG_514690) {
  EXPECT_TRUE(OpenDocument("hello_world.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);

  // Test that FORM_OnMouseMove() etc. permit null HANDLES and PAGES.
  FORM_OnMouseMove(nullptr, page, 0, 10.0, 10.0);
  FORM_OnMouseMove(form_handle(), nullptr, 0, 10.0, 10.0);

  UnloadPage(page);
}

#ifdef PDF_ENABLE_V8
TEST_F(FPDFFormFillEmbeddertest, BUG_551248) {
  // Test that timers fire once and intervals fire repeatedly.
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_551248.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  DoOpenActions();

  const auto& alerts = delegate.GetAlerts();
  EXPECT_EQ(0U, alerts.size());

  delegate.AdvanceTime(1000);
  EXPECT_EQ(0U, alerts.size());  // nothing fired.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(1U, alerts.size());  // interval fired.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(2U, alerts.size());  // timer fired.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(3U, alerts.size());  // interval fired again.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(3U, alerts.size());  // nothing fired.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(4U, alerts.size());  // interval fired again.
  delegate.AdvanceTime(1000);
  EXPECT_EQ(4U, alerts.size());  // nothing fired.
  UnloadPage(page);

  ASSERT_EQ(4U, alerts.size());  // nothing else fired.

  EXPECT_STREQ(L"interval fired", alerts[0].message.c_str());
  EXPECT_STREQ(L"Alert", alerts[0].title.c_str());
  EXPECT_EQ(0, alerts[0].type);
  EXPECT_EQ(0, alerts[0].icon);

  EXPECT_STREQ(L"timer fired", alerts[1].message.c_str());
  EXPECT_STREQ(L"Alert", alerts[1].title.c_str());
  EXPECT_EQ(0, alerts[1].type);
  EXPECT_EQ(0, alerts[1].icon);

  EXPECT_STREQ(L"interval fired", alerts[2].message.c_str());
  EXPECT_STREQ(L"Alert", alerts[2].title.c_str());
  EXPECT_EQ(0, alerts[2].type);
  EXPECT_EQ(0, alerts[2].icon);

  EXPECT_STREQ(L"interval fired", alerts[3].message.c_str());
  EXPECT_STREQ(L"Alert", alerts[3].title.c_str());
  EXPECT_EQ(0, alerts[3].type);
  EXPECT_EQ(0, alerts[3].icon);
}

TEST_F(FPDFFormFillEmbeddertest, BUG_620428) {
  // Test that timers and intervals are cancelable.
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_620428.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  DoOpenActions();
  delegate.AdvanceTime(5000);
  UnloadPage(page);

  const auto& alerts = delegate.GetAlerts();
  ASSERT_EQ(1U, alerts.size());
  EXPECT_STREQ(L"done", alerts[0].message.c_str());
}

TEST_F(FPDFFormFillEmbeddertest, BUG_634394) {
  // Cancel timer inside timer callback.
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_634394.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  DoOpenActions();

  // Timers fire at most once per AdvanceTime(), allow intervals
  // to fire several times if possible.
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  UnloadPage(page);

  const auto& alerts = delegate.GetAlerts();
  EXPECT_EQ(2U, alerts.size());
}

TEST_F(FPDFFormFillEmbeddertest, BUG_634716) {
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_634716.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);
  DoOpenActions();

  // Timers fire at most once per AdvanceTime(), allow intervals
  // to fire several times if possible.
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  delegate.AdvanceTime(1000);
  UnloadPage(page);

  const auto& alerts = delegate.GetAlerts();
  EXPECT_EQ(2U, alerts.size());
}

TEST_F(FPDFFormFillEmbeddertest, BUG_679649) {
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_679649.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);

  delegate.SetFailNextTimer();
  DoOpenActions();
  delegate.AdvanceTime(2000);
  UnloadPage(page);

  const auto& alerts = delegate.GetAlerts();
  EXPECT_EQ(0u, alerts.size());
}

TEST_F(FPDFFormFillEmbeddertest, BUG_707673) {
  EmbedderTestTimerHandlingDelegate delegate;
  SetDelegate(&delegate);

  EXPECT_TRUE(OpenDocument("bug_707673.pdf"));
  FPDF_PAGE page = LoadPage(0);
  EXPECT_TRUE(page);

  DoOpenActions();
  FORM_OnLButtonDown(form_handle(), page, 0, 140, 590);
  FORM_OnLButtonUp(form_handle(), page, 0, 140, 590);
  delegate.AdvanceTime(1000);
  UnloadPage(page);

  const auto& alerts = delegate.GetAlerts();
  EXPECT_EQ(0u, alerts.size());
}

#endif  // PDF_ENABLE_V8

TEST_F(FPDFFormFillEmbeddertest, FormText) {
#if _FXM_PLATFORM_ == _FXM_PLATFORM_APPLE_
  const char md5_1[] = "5f11dbe575fe197a37c3fb422559f8ff";
  const char md5_2[] = "35b1a4b679eafc749a0b6fda750c0e8d";
  const char md5_3[] = "65c64a7c355388f719a752aa1e23f6fe";
#else
  const char md5_1[] = "23baecc6e94d4c8b894cd39aa04c584c";
  const char md5_2[] = "499df95d477dfe35ee65b823c69743b5";
  const char md5_3[] = "8f91b62895fc505d9e17ff2d633756d4";
#endif
  {
    EXPECT_TRUE(OpenDocument("text_form.pdf"));
    FPDF_PAGE page = LoadPage(0);
    ASSERT_TRUE(page);
    std::unique_ptr<void, FPDFBitmapDeleter> bitmap1(RenderPage(page));
    CompareBitmap(bitmap1.get(), 300, 300, md5_1);

    // Click on the textfield
    EXPECT_EQ(FPDF_FORMFIELD_TEXTFIELD,
              FPDFPage_HasFormFieldAtPoint(form_handle(), page, 120.0, 120.0));
    FORM_OnMouseMove(form_handle(), page, 0, 120.0, 120.0);
    FORM_OnLButtonDown(form_handle(), page, 0, 120.0, 120.0);
    FORM_OnLButtonUp(form_handle(), page, 0, 120.0, 120.0);

    // Write "ABC"
    FORM_OnChar(form_handle(), page, 65, 0);
    FORM_OnChar(form_handle(), page, 66, 0);
    FORM_OnChar(form_handle(), page, 67, 0);
    std::unique_ptr<void, FPDFBitmapDeleter> bitmap2(RenderPage(page));
    CompareBitmap(bitmap2.get(), 300, 300, md5_2);

    // Take out focus by clicking out of the textfield
    FORM_OnMouseMove(form_handle(), page, 0, 15.0, 15.0);
    FORM_OnLButtonDown(form_handle(), page, 0, 15.0, 15.0);
    FORM_OnLButtonUp(form_handle(), page, 0, 15.0, 15.0);
    std::unique_ptr<void, FPDFBitmapDeleter> bitmap3(RenderPage(page));
    CompareBitmap(bitmap3.get(), 300, 300, md5_3);

    EXPECT_TRUE(FPDF_SaveAsCopy(document(), this, 0));

    // Close everything
    UnloadPage(page);
    FPDFDOC_ExitFormFillEnvironment(form_handle_);
    FPDF_CloseDocument(document_);
  }
  // Check saved document
  std::string new_file = GetString();
  FPDF_FILEACCESS file_access;
  memset(&file_access, 0, sizeof(file_access));
  file_access.m_FileLen = new_file.size();
  file_access.m_GetBlock = GetBlockFromString;
  file_access.m_Param = &new_file;
  document_ = FPDF_LoadCustomDocument(&file_access, nullptr);
  SetupFormFillEnvironment();
  EXPECT_EQ(1, FPDF_GetPageCount(document_));
  std::unique_ptr<void, FPDFPageDeleter> new_page(FPDF_LoadPage(document_, 0));
  ASSERT_TRUE(new_page.get());
  std::unique_ptr<void, FPDFBitmapDeleter> new_bitmap(
      RenderPage(new_page.get()));
  CompareBitmap(new_bitmap.get(), 300, 300, md5_3);
}

TEST_F(FPDFFormFillEmbeddertest, GetSelectedTextEmptyAndBasicKeyboard) {
  // Open file with form text field.
  EXPECT_TRUE(OpenDocument("text_form.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Test empty selection.
  CheckSelection(page, CFX_WideString(L""));

  // Test basic selection.
  TypeTextIntoTextfield(page, 3);
  SelectTextWithKeyboard(page, 3, FWL_VKEY_Left, 123.0, 115.5);
  CheckSelection(page, CFX_WideString(L"ABC"));

  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, GetSelectedTextEmptyAndBasicMouse) {
  // Open file with form text field.
  EXPECT_TRUE(OpenDocument("text_form.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  // Test empty selection.
  CheckSelection(page, CFX_WideString(L""));

  // Test basic selection.
  TypeTextIntoTextfield(page, 3);
  SelectTextWithMouse(page, 125.0, 102.0, 115.5);
  CheckSelection(page, CFX_WideString(L"ABC"));

  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, GetSelectedTextFragmentsKeyBoard) {
  // Open file with form text field.
  EXPECT_TRUE(OpenDocument("text_form.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  TypeTextIntoTextfield(page, 12);

  // Test selecting first character in forward direction.
  // Navigate to starting position and click mouse.
  SelectTextWithKeyboard(page, 1, FWL_VKEY_Right, 102.0, 115.5);
  CheckSelection(page, CFX_WideString(L"A"));

  // Test selecting entire long string in backwards direction.
  SelectTextWithKeyboard(page, 12, FWL_VKEY_Left, 191.0, 115.5);
  CheckSelection(page, CFX_WideString(L"ABCDEFGHIJKL"));

  // Test selecting middle section in backwards direction.
  SelectTextWithKeyboard(page, 6, FWL_VKEY_Left, 170.0, 115.5);
  CheckSelection(page, CFX_WideString(L"DEFGHI"));

  // Test selecting middle selection in forward direction.
  SelectTextWithKeyboard(page, 6, FWL_VKEY_Right, 125.0, 115.5);
  CheckSelection(page, CFX_WideString(L"DEFGHI"));

  // Test selecting last character in backwards direction.
  SelectTextWithKeyboard(page, 1, FWL_VKEY_Left, 191.0, 115.5);
  CheckSelection(page, CFX_WideString(L"L"));

  UnloadPage(page);
}

TEST_F(FPDFFormFillEmbeddertest, GetSelectedTextFragmentsMouse) {
  // Open file with form text field.
  EXPECT_TRUE(OpenDocument("text_form.pdf"));
  FPDF_PAGE page = LoadPage(0);
  ASSERT_TRUE(page);

  TypeTextIntoTextfield(page, 12);

  // Test selecting first character in forward direction.
  SelectTextWithMouse(page, 102.0, 106.0, 115.5);
  CheckSelection(page, CFX_WideString(L"A"));

  // Test selecting entire long string in backwards direction.
  SelectTextWithMouse(page, 191.0, 102.0, 115.5);
  CheckSelection(page, CFX_WideString(L"ABCDEFGHIJKL"));

  // Test selecting middle section in backwards direction.
  SelectTextWithMouse(page, 170.0, 125.0, 115.5);
  CheckSelection(page, CFX_WideString(L"DEFGHI"));

  // Test selecting middle selection in forward direction.
  SelectTextWithMouse(page, 125.0, 170.0, 115.5);
  CheckSelection(page, CFX_WideString(L"DEFGHI"));

  // Test selecting last character in backwards direction.
  SelectTextWithMouse(page, 191.0, 186.0, 115.5);
  CheckSelection(page, CFX_WideString(L"L"));

  UnloadPage(page);
}
