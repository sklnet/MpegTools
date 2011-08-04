@echo off
rem
rem $Id$
rem Copyright (C) 2011 gingko - http://gingko.homeip.net/
rem
rem This file is part of Pouchin TV Mod, a free DVB-T viewer.
rem See http://www.pouchintv.fr/ for updates.

rem Construction de l'ensemble des fichiers r‚f‚rant au num‚ro de version pour un projet donn‚
rem Les num‚ros eux-mˆmes sont d‚finis dans le fichier "version_root_defs.bat', situ‚ dans le mˆme r‚pertoire.

rem S'assurer que le r‚pertoire courant est bien celui o— se trouve le pr‚sent script
%~d0
cd %~p0

rem D‚truire les fichiers de versions gˆnants h‚rit‚s de versions plus anciennes du projet.
if exist PTvM_Lib\version.h del PTvM_Lib\version.h
if exist PTvM_Lib\version.nsh del PTvM_Lib\version.nsh

rem R‚cup‚rer les num‚ros de version de base
call version_root_defs.bat

..\..\version\fetch_svn_version.bat "h,bat"
