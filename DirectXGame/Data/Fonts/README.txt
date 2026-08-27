このフォルダに日本語フォントの実体を置く。

  JapaneseSans.ttf      本文・UI(ゴシック 標準)
  JapaneseSansBold.ttf  見出し・強調(ゴシック 太字)
  JapaneseSerif.ttf     世界観の文字(明朝)

シーンやPrefabのJSONは上の**論理名**(Fonts/JapaneseSans.ttf など)だけを書く。
実体をここへ置けばそれが使われ、置いていなければ見た目の近いシステムフォントへ
自動で落ちる(KujataEngine/2d/FontAtlas.cpp の ResolveFontPath)。

**同梱してよいフォントだけを置くこと。** Windows付属フォント(游明朝・游ゴシック等)は
再配布が許諾されていないため、ここへコピーしてはいけない。
再配布が明示的に許諾されているものを使う:
  - IPAexフォント(IPAexゴシック / IPAex明朝)  IPAフォントライセンス v1.0
  - Noto Sans JP / Noto Serif JP              SIL Open Font License 1.1
  - Source Han Sans / Serif                   SIL Open Font License 1.1
拡張子が .otf / .ttc の場合も、上の名前へリネームして置けば読める
(stb_truetypeがTTF/TTCを直接読む。可変フォントは非対応なので静的なウェイトを使う)。

ライセンス本文も一緒にこのフォルダへ置いておくこと。
