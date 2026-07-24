# GameModule

ゲーム側コード(`../GameComponents/` の Component と `SampleScene`)を
**Hot Reload 用の DLL** (`GameModule.dll`) にまとめるプロジェクトです。

- ソースはこのフォルダと `../GameComponents/` にあります。
- ビルド出力は `bin/<構成>/GameModule.dll`(git 管理外)。エンジン (`KujakuEngine.exe`) が起動時にここから読み込みます。
- エディタの `Reload DLL` を押すと、エンジンが MSBuild でこのプロジェクトを `../Temp/HotReload/` 配下へ世代別にビルドし直して差し替えます。

注意: exe と DLL は同じ構成 (Debug/Release) でビルドしないと STL の ABI が食い違ってクラッシュします。
Component の追加・変更時は `.sln` 経由で exe と DLL を同時にビルドしてください。
詳細はリポジトリ直下 `ReadMe.md` の「DLL Hot Reload」章を参照。
