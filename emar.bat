:: Entry point for running python scripts on windows systems.
::
:: Automatically generated by `create_entry_points.py`; DO NOT EDIT.
::
:: To make modifications to this file, edit `tools/run_python.bat` and then run
:: `tools/create_entry_points.py`

:: All env. vars specified in this file are to be local only to this script.
@setlocal

@set PYTHONPATH=%~dp0\.pip

@set EM_PY=%EMSDK_PYTHON%
@if "%EM_PY%"=="" (
  set EM_PY=python
)

:: Python Windows bug https://bugs.python.org/issue34780: If this script was invoked via a
:: shared stdin handle from the parent process, and that parent process stdin handle is in
:: a certain state, running python.exe might hang here. To work around this, if
:: EM_WORKAROUND_PYTHON_BUG_34780 is defined, invoke python with '< NUL' stdin to avoid
:: sharing the parent's stdin handle to it, avoiding the hang.

:: On Windows 7, the compiler batch scripts are observed to exit with a non-zero errorlevel,
:: even when the python executable above did succeed and quit with errorlevel 0 above.
:: On Windows 8 and newer, this issue has not been observed. It is possible that this
:: issue is related to the above python bug, but this has not been conclusively confirmed,
:: so using a separate env. var EM_WORKAROUND_WIN7_BAD_ERRORLEVEL_BUG to enable the known
:: workaround this issue, which is to explicitly quit the calling process with the previous
:: errorlevel from the above command.

:: Also must use goto to jump to the command dispatch, since we cannot invoke emcc from
:: inside a if() block, because if a cmdline param would contain a char '(' or ')', that
:: would throw off the parsing of the cmdline arg.
@if "%EM_WORKAROUND_PYTHON_BUG_34780%"=="" (
  @if "%EM_WORKAROUND_WIN7_BAD_ERRORLEVEL_BUG%"=="" (
    goto NORMAL
  ) else (
    goto NORMAL_EXIT
  )
) else (
  @if "%EM_WORKAROUND_WIN7_BAD_ERRORLEVEL_BUG%"=="" (
    goto MUTE_STDIN
  ) else (
    goto MUTE_STDIN_EXIT
  )
)

:NORMAL_EXIT
@"%EM_PY%" "%~dp0\%~n0.py" %*
@exit %ERRORLEVEL%

:MUTE_STDIN
@"%EM_PY%" "%~dp0\%~n0.py" %* < NUL
@exit /b %ERRORLEVEL%

:MUTE_STDIN_EXIT
@"%EM_PY%" "%~dp0\%~n0.py" %* < NUL
@exit %ERRORLEVEL%

:NORMAL
@"%EM_PY%" "%~dp0\%~n0.py" %*
