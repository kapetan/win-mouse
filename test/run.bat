@echo off

set dirname=%~dp0

%1 %dirname%parallel-destroy || goto :error
%1 %dirname%parallel-unref || goto :error
%1 %dirname%sequence-destroy || goto :error
%1 %dirname%sequence-unref || goto :error

:error
exit /b %errorlevel%
