# eventually use this to find UE install
#$UE_DIR = (Get-ItemProperty 'Registry::HKEY_LOCAL_MACHINE\SOFTWARE\EpicGames\Unreal Engine\5.3' -Name 'InstalledDirectory' ).'InstalledDirectory'

C:\UE\5.4\Engine\Build\BatchFiles\RunUAT.bat -Verbose BuildPlugin -Plugin="C:\Git\rive\packages\runtime_unreal\Plugins\Rive\Rive.uplugin" -Package="C:\Git\tmp\Rive" -precompile -TargetPlatforms=Win64+Android