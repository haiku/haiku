/* Copyright 1992 NEC Corporation, Tokyo, Japan.
 *
 * Permission to use, copy, modify, distribute and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of NEC
 * Corporation not be used in advertising or publicity pertaining to
 * distribution of the software without specific, written prior
 * permission.  NEC Corporation makes no representations about the
 * suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * NEC CORPORATION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN 
 * NO EVENT SHALL NEC CORPORATION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF 
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR 
 * OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR 
 * PERFORMANCE OF THIS SOFTWARE. 
 */

#ifndef _MFDEF_H_
#define _MFDEF_H_

/* @(#) 102.1 $Id: mfdef.h 10527 2004-12-23 22:08:39Z korli $ */

/*
  このヘッダファイルではモードに割り振られた番号や、関数に割り振られた
  番号を管理する。

  */

/* モード */

/* real modes */
/* 実モード(real mode)はキーマップの実体を持っているモード */

#define CANNA_MODE_AlphaMode		0	/* アルファベットモード */
#define CANNA_MODE_EmptyMode		1	/* 読み入力がない状態 */
#define CANNA_MODE_KigoMode		2	/* 記号一覧表示状態 */
#define CANNA_MODE_YomiMode		3	/* 読み入力している状態 */
#define CANNA_MODE_JishuMode		4	/* 文字種変換している状態 */
#define CANNA_MODE_TankouhoMode		5	/* 単一候補表示状態 */
#define CANNA_MODE_IchiranMode		6	/* 候補一覧表示状態 */
#define CANNA_MODE_YesNoMode		7	/* 単語登録の例文表示状態 */
#define CANNA_MODE_OnOffMode		8	/* 単語登録の例文表示状態 */
#define CANNA_MODE_AdjustBunsetsuMode   9	/* 文節伸縮モード */
#define CANNA_MODE_ChikujiYomiMode	10	/* 逐次変換モードの読み部分 */
#define CANNA_MODE_ChikujiTanMode	11	/* 逐次変換モードの候補部分 */

#define CANNA_MODE_MAX_REAL_MODE	(CANNA_MODE_ChikujiTanMode + 1)

/* imaginary modes */
/* 虚モード(imaginary mode)はキーマップの実体を持っていないモード */

#define CANNA_MODE_HenkanMode		CANNA_MODE_EmptyMode
#define CANNA_MODE_HenkanNyuryokuMode	12

#define CANNA_MODE_ZenHiraHenkanMode	13
#define CANNA_MODE_HanHiraHenkanMode	14
#define CANNA_MODE_ZenKataHenkanMode	15
#define CANNA_MODE_HanKataHenkanMode	16
#define CANNA_MODE_ZenAlphaHenkanMode	17
#define CANNA_MODE_HanAlphaHenkanMode	18

#define CANNA_MODE_ZenHiraKakuteiMode	19
#define CANNA_MODE_HanHiraKakuteiMode	20
#define CANNA_MODE_ZenKataKakuteiMode	21
#define CANNA_MODE_HanKataKakuteiMode	22
#define CANNA_MODE_ZenAlphaKakuteiMode	23
#define CANNA_MODE_HanAlphaKakuteiMode	24

#define CANNA_MODE_HexMode		25	/* １６進コード入力モード */
#define CANNA_MODE_BushuMode		26	/* 部首の読みの入力状態 */
#define CANNA_MODE_ExtendMode		27	/* 拡張機能選択 */
#define CANNA_MODE_RussianMode		28	/* ロシア文字選択 */
#define CANNA_MODE_GreekMode		29	/* ギリシア文字選択 */
#define CANNA_MODE_LineMode		30	/* 罫線選択 */
#define CANNA_MODE_ChangingServerMode	31	/* サーバ変更 */
#define CANNA_MODE_HenkanMethodMode	32	/* 変換方式選択 */
#define CANNA_MODE_DeleteDicMode	33	/* 単語削除 */
#define CANNA_MODE_TourokuMode		34	/* 単語登録モード */
#define CANNA_MODE_TourokuEmptyMode	CANNA_MODE_TourokuMode
#define CANNA_MODE_TourokuHinshiMode	35	/* 単語登録の品詞選択状態 */
#define CANNA_MODE_TourokuDicMode	36	/* 単語登録の辞書選択状態 */
#define CANNA_MODE_QuotedInsertMode	37	/* 引用入力モード */
#define CANNA_MODE_BubunMuhenkanMode	38	/* 部分無変換状態 */
#define CANNA_MODE_MountDicMode   	39	/* 辞書のmount,unmount状態 */

#define CANNA_MODE_MAX_IMAGINARY_MODE	(CANNA_MODE_MountDicMode + 1)

#ifdef IROHA_BC
/* real modes */
/* 実モード(real mode)はキーマップの実体を持っているモード */

#define IROHA_MODE_AlphaMode		CANNA_MODE_AlphaMode
#define IROHA_MODE_EmptyMode		CANNA_MODE_EmptyMode
#define IROHA_MODE_KigoMode		CANNA_MODE_KigoMode
#define IROHA_MODE_YomiMode		CANNA_MODE_YomiMode
#define IROHA_MODE_JishuMode		CANNA_MODE_JishuMode
#define IROHA_MODE_TankouhoMode		CANNA_MODE_TankouhoMode
#define IROHA_MODE_IchiranMode		CANNA_MODE_IchiranMode
#define IROHA_MODE_YesNoMode		CANNA_MODE_YesNoMode
#define IROHA_MODE_OnOffMode		CANNA_MODE_OnOffMode
#define IROHA_MODE_AdjustBunsetsuMode   CANNA_MODE_AdjustBunsetsuMode

#define IROHA_MODE_MAX_REAL_MODE	CANNA_MODE_MAX_REAL_MODE

/* imaginary modes */
/* 虚モード(imaginary mode)はキーマップの実体を持っていないモード */

#define IROHA_MODE_HenkanMode		CANNA_MODE_HenkanMode
#define IROHA_MODE_HenkanNyuryokuMode	CANNA_MODE_HenkanNyuryokuMode
#define IROHA_MODE_HexMode		CANNA_MODE_HexMode
#define IROHA_MODE_BushuMode		CANNA_MODE_BushuMode
#define IROHA_MODE_ExtendMode		CANNA_MODE_ExtendMode
#define IROHA_MODE_RussianMode		CANNA_MODE_RussianMode
#define IROHA_MODE_GreekMode		CANNA_MODE_GreekMode
#define IROHA_MODE_LineMode		CANNA_MODE_LineMode
#define IROHA_MODE_ChangingServerMode	CANNA_MODE_ChangingServerMode
#define IROHA_MODE_HenkanMethodMode	CANNA_MODE_HenkanMethodMode
#define IROHA_MODE_DeleteDicMode	CANNA_MODE_DeleteDicMode
#define IROHA_MODE_TourokuMode		CANNA_MODE_TourokuMode
#define IROHA_MODE_TourokuEmptyMode	CANNA_MODE_TourokuEmptyMode
#define IROHA_MODE_TourokuHinshiMode	CANNA_MODE_TourokuHinshiMode
#define IROHA_MODE_TourokuDicMode	CANNA_MODE_TourokuDicMode
#define IROHA_MODE_QuotedInsertMode	CANNA_MODE_QuotedInsertMode
#define IROHA_MODE_BubunMuhenkanMode	CANNA_MODE_BubunMuhenkanMode
#define IROHA_MODE_MountDicMode   	CANNA_MODE_MountDicMode

#define IROHA_MODE_ZenHiraHenkanMode	CANNA_MODE_ZenHiraHenkanMode
#define IROHA_MODE_HanHiraHenkanMode	CANNA_MODE_HanHiraHenkanMode
#define IROHA_MODE_ZenKataHenkanMode	CANNA_MODE_ZenKataHenkanMode
#define IROHA_MODE_HanKataHenkanMode	CANNA_MODE_HanKataHenkanMode
#define IROHA_MODE_ZenAlphaHenkanMode	CANNA_MODE_ZenAlphaHenkanMode
#define IROHA_MODE_HanAlphaHenkanMode	CANNA_MODE_HanAlphaHenkanMode

#define IROHA_MODE_ZenHiraKakuteiMode	CANNA_MODE_ZenHiraKakuteiMode
#define IROHA_MODE_HanHiraKakuteiMode	CANNA_MODE_HanHiraKakuteiMode
#define IROHA_MODE_ZenKataKakuteiMode	CANNA_MODE_ZenKataKakuteiMode
#define IROHA_MODE_HanKataKakuteiMode	CANNA_MODE_HanKataKakuteiMode
#define IROHA_MODE_ZenAlphaKakuteiMode	CANNA_MODE_ZenAlphaKakuteiMode
#define IROHA_MODE_HanAlphaKakuteiMode	CANNA_MODE_HanAlphaKakuteiMode

#define IROHA_MODE_MAX_IMAGINARY_MODE	CANNA_MODE_MAX_IMAGINARY_MODE

#endif /* IROHA_BC */

/* キー関数 */

/* 未定義にする */

#define DEFAULTBEHAVIOR 0
#define CANNA_FN_Undefined		0 /* 未定義 */

/* 文字挿入 */

#define CANNA_FN_SelfInsert		1 /* 押されたキーをそのまま挿入 */
#define CANNA_FN_FunctionalInsert	2 /* 押されたキーの挿入 */
#define CANNA_FN_QuotedInsert		3 /* 次の文字をそのまま挿入 */

/* 基本的モード切り換え */

#define CANNA_FN_JapaneseMode		4 /* 日本語入力モードへ移行 */
#define CANNA_FN_AlphaMode		5 /* アルファベット入力モードへ移行 */
#define CANNA_FN_HenkanNyuryokuMode	6 /* 変換入力モードへ移行 */

/* 編集 */

#define CANNA_FN_Forward		7 /* 右 */
#define CANNA_FN_Backward		8 /* 左 */
#define CANNA_FN_Next			9 /* 次の行 */
#define CANNA_FN_Prev			10 /* 前の行 */
#define CANNA_FN_BeginningOfLine	11 /* 行頭 */
#define CANNA_FN_EndOfLine		12 /* 行末 */
#define CANNA_FN_DeleteNext		13 /* 右削除 */
#define CANNA_FN_DeletePrevious		14 /* 左削除 */
#define CANNA_FN_KillToEndOfLine	15 /* 行末まで削除 */

/* カナ漢字変換機能 */

#define CANNA_FN_Henkan			16 /* 変換 */
#define CANNA_FN_Kakutei		17 /* 確定 */
#define CANNA_FN_Extend			18 /* 伸ばし */
#define CANNA_FN_Shrink			19 /* 縮め */
#define CANNA_FN_AdjustBunsetsu		20 /* 文節伸縮モードに入る */
#define CANNA_FN_Quit			21 /* 取りやめ */
#define CANNA_FN_ConvertAsHex		22 /* １６進コードとして変換 */
#define CANNA_FN_ConvertAsBushu		23 /* 部首名として変換 */
#define CANNA_FN_KouhoIchiran		24 /* 候補一覧 */
#define CANNA_FN_BubunMuhenkan		25 /* 部分無変換 */

/* 文字種変換だけに使われる機能 */

#define CANNA_FN_Zenkaku		26 /* 全角 */
#define CANNA_FN_Hankaku		27 /* 半角 */
#define CANNA_FN_ToUpper		28 /* 大文字 */
#define CANNA_FN_Capitalize		29 /* 先頭だけ大文字に */
#define CANNA_FN_ToLower		30 /* 小文字 */
#define CANNA_FN_Hiragana		31 /* ひらがな */
#define CANNA_FN_Katakana		32 /* カタカナ */
#define CANNA_FN_Romaji			33 /* ローマ字 */

/* 読みモードでのベース文字の切り替え */

#define CANNA_FN_BaseHiragana		34 /* ひらがなモード */
#define CANNA_FN_BaseKatakana		35 /* カタカナモード */
#define CANNA_FN_BaseEisu		36 /* 英数モード */
#define CANNA_FN_BaseZenkaku		37 /* 全角モード */
#define CANNA_FN_BaseHankaku		38 /* 半角モード */
#define CANNA_FN_BaseKana		39 /* かなモード */
#define CANNA_FN_BaseKakutei		40 /* 確定入力モード */
#define CANNA_FN_BaseHenkan		41 /* 変換入力モード */
#define CANNA_FN_BaseHiraKataToggle	42 /* ひらがなモード/カタカナモード */
#define CANNA_FN_BaseZenHanToggle	43 /* 全角モード/半角モード */
#define CANNA_FN_BaseKanaEisuToggle	44 /* かなモード/英数モード */
#define CANNA_FN_BaseKakuteiHenkanToggle 45 /* 確定入力モード/変換入力モード */
#define CANNA_FN_BaseRotateForward	46 /* モードを順繰りに切り替える */
#define CANNA_FN_BaseRotateBackward	47 /* モードを逆繰りに切り替える */

/* その他のモード切り替え */

#define CANNA_FN_ExtendMode		48 /* 拡張モード */
#define CANNA_FN_Touroku                CANNA_FN_ExtendMode /* 旧互換 */

#define CANNA_FN_HexMode		49 /* １６進入力モード */
#define CANNA_FN_BushuMode		50 /* 部首入力モード */
#define CANNA_FN_KigouMode		51 /* 記号入力モード */

#define CANNA_FN_Mark			52 /* マーク */
#define CANNA_FN_TemporalMode		53 /* モードの一時的セーブ */
#define CANNA_FN_UserMode		54 /* ユーザ定義モード */

/* 現在の表示を返して欲しいときに使う機能 */

#define CANNA_FN_Nop			55 /* 何もしない */

/* Version 2.1 には入っていない機能 */

#define CANNA_FN_HenkanOrInsert		56 /* 変換もしくはそのまま入力 */
#define CANNA_FN_HenkanNaive		CANNA_FN_HenkanOrInsert

/* Version 2.2 には入っていない機能 */

#define CANNA_FN_HenkanOrNothing	57 /* 変換もしくはなにもしない */
#define CANNA_FN_RussianMode		58 /* ロシア文字モード */
#define CANNA_FN_GreekMode		59 /* ギリシャ文字モード */
#define CANNA_FN_LineMode		60 /* 罫線モード */
#define CANNA_FN_DicMountMode		61 /* 辞書選択モード */
#define CANNA_FN_ChangeServerMode	62 /* サーバ選択モード */
#define CANNA_FN_DisconnectServer	63 /* サーバ切り離し */
#define CANNA_FN_ShowServer		64 /* サーバ表示 */
#define CANNA_FN_ShowGakushu		65 /* 学習状況表示 */
#define CANNA_FN_ShowVersion		66 /* バージョン表示 */
#define CANNA_FN_ShowPhonogramFile	67 /* ローマ字ファイル表示 */
#define CANNA_FN_ShowCannaFile		68 /* カスタマイズファイル表示 */
#define CANNA_FN_DefineDicMode		69 /* 単語登録 */
#define CANNA_FN_DeleteDicMode		70 /* 単語削除 */
#define CANNA_FN_SyncDic		71 /* 辞書書き込み */
#define CANNA_FN_EnterChikujiMode	72 /* 逐次自動変換モード */
#define CANNA_FN_EnterRenbunMode	73 /* 連文節変換モード */

/* Version 3.5 で追加されたもの */
#define CANNA_FN_PageUp                 74 /* ページアップ */
#define CANNA_FN_PageDown               75 /* ページダウン */
#define CANNA_FN_Edit			76 /* 編集 */
#define CANNA_FN_BubunKakutei		77 /* カーソル以前確定 */

#define CANNA_FN_HenkanRegion		78 /* 選択領域の変換 */
#define CANNA_FN_PhonoEdit		79 /* ローマ字エディタ */
#define CANNA_FN_DicEdit		80 /* 辞書エディタ */
#define CANNA_FN_Configure		81 /* Canna の設定 */

#define CANNA_FN_KanaRotate		82 /* カナ字種変換 */
#define CANNA_FN_RomajiRotate		83 /* 英数字種変換 */
#define CANNA_FN_CaseRotate		84 /* 大文字小文字変換 */

/* 以下の２つは必ず最後に置くこと。ccustom がらみの理由から */

#define CANNA_FN_FuncSequence		85 /* 複数の機能の割り当て */
#define CANNA_FN_UseOtherKeymap		86 /* キーシーケンスの対応 */

#define CANNA_FN_MAX_FUNC		(CANNA_FN_UseOtherKeymap + 1)

#ifdef IROHA_BC

#define IROHA_FN_Undefined		CANNA_FN_Undefined

/* 文字挿入 */

#define IROHA_FN_SelfInsert		CANNA_FN_SelfInsert
#define IROHA_FN_FunctionalInsert	CANNA_FN_FunctionalInsert
#define IROHA_FN_QuotedInsert		CANNA_FN_QuotedInsert

/* 基本的モード切り換え */

#define IROHA_FN_JapaneseMode		CANNA_FN_JapaneseMode
#define IROHA_FN_AlphaMode		CANNA_FN_AlphaMode
#define IROHA_FN_HenkanNyuryokuMode	CANNA_FN_HenkanNyuryokuMode


/* 編集 */

#define IROHA_FN_Forward		CANNA_FN_Forward
#define IROHA_FN_Backward		CANNA_FN_Backward
#define IROHA_FN_Next			CANNA_FN_Next
#define IROHA_FN_Prev			CANNA_FN_Prev
#define IROHA_FN_BeginningOfLine	CANNA_FN_BeginningOfLine
#define IROHA_FN_EndOfLine		CANNA_FN_EndOfLine
#define IROHA_FN_DeleteNext		CANNA_FN_DeleteNext
#define IROHA_FN_DeletePrevious		CANNA_FN_DeletePrevious
#define IROHA_FN_KillToEndOfLine	CANNA_FN_KillToEndOfLine

/* カナ漢字変換機能 */

#define IROHA_FN_Henkan			CANNA_FN_Henkan
#define IROHA_FN_Kakutei		CANNA_FN_Kakutei
#define IROHA_FN_Extend			CANNA_FN_Extend
#define IROHA_FN_Shrink			CANNA_FN_Shrink
#define IROHA_FN_AdjustBunsetsu		CANNA_FN_AdjustBunsetsu
#define IROHA_FN_Quit			CANNA_FN_Quit
#define IROHA_FN_ConvertAsHex		CANNA_FN_ConvertAsHex
#define IROHA_FN_ConvertAsBushu		CANNA_FN_ConvertAsBushu
#define IROHA_FN_KouhoIchiran		CANNA_FN_KouhoIchiran
#define IROHA_FN_BubunMuhenkan		CANNA_FN_BubunMuhenkan

/* 文字種変換だけに使われる機能 */

#define IROHA_FN_Zenkaku		CANNA_FN_Zenkaku
#define IROHA_FN_Hankaku		CANNA_FN_Hankaku
#define IROHA_FN_ToUpper		CANNA_FN_ToUpper
#define IROHA_FN_Capitalize		CANNA_FN_Capitalize
#define IROHA_FN_ToLower		CANNA_FN_ToLower
#define IROHA_FN_Hiragana		CANNA_FN_Hiragana
#define IROHA_FN_Katakana		CANNA_FN_Katakana
#define IROHA_FN_Romaji			CANNA_FN_Romaji

/* その他のモード切り替え */

#define IROHA_FN_ExtendMode		CANNA_FN_ExtendMode
#define IROHA_FN_Touroku                CANNA_FN_Touroku

#define IROHA_FN_HexMode		CANNA_FN_HexMode
#define IROHA_FN_BushuMode		CANNA_FN_BushuMode
#define IROHA_FN_KigouMode		CANNA_FN_KigouMode

#define IROHA_FN_UserMode		CANNA_FN_UserMode

/* 複雑な機能 */

#define IROHA_FN_FuncSequence		CANNA_FN_FuncSequence
#define IROHA_FN_UseOtherKeymap		CANNA_FN_UseOtherKeymap

#define IROHA_FN_MAX_FUNC		CANNA_FN_MAX_FUNC

#endif /* IROHA_BC */

#endif /* _MFDEF_H_ */
