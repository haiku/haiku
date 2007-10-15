#include "rar.hpp"




bool MergeArchive(Archive &Arc,ComprDataIO *DataIO,bool ShowFileName,char Command)
{
  RAROptions *Cmd=Arc.GetRAROptions();

  int HeaderType=Arc.GetHeaderType();
  FileHeader *hd=HeaderType==NEWSUB_HEAD ? &Arc.SubHead:&Arc.NewLhd;
  bool SplitHeader=(HeaderType==FILE_HEAD || HeaderType==NEWSUB_HEAD) &&
                   (hd->Flags & LHD_SPLIT_AFTER)!=0;

  if (DataIO!=NULL && SplitHeader && hd->UnpVer>=20 &&
      hd->FileCRC!=0xffffffff && DataIO->PackedCRC!=~hd->FileCRC)
  {
    Log(Arc.FileName,St(MDataBadCRC),hd->FileName,Arc.FileName);
  }

  Int64 PosBeforeClose=Arc.Tell();
  Arc.Close();

  char NextName[NM];
  wchar NextNameW[NM];
  *NextNameW=0;
  strcpy(NextName,Arc.FileName);
  NextVolumeName(NextName,(Arc.NewMhd.Flags & MHD_NEWNUMBERING)==0 || Arc.OldFormat);

  if (*Arc.FileNameW!=0)
  {
    // Copy incremented trailing low ASCII volume name part to Unicode name.
    // It is simpler than also implementing Unicode version of NextVolumeName.

    strcpyw(NextNameW,Arc.FileNameW);
    char *NumPtr=GetVolNumPart(NextName);

    // moving to first digit in volume number
    while (NumPtr>NextName && isdigit(*NumPtr) && isdigit(*(NumPtr-1)))
      NumPtr--;

    // also copy the first character before volume number,
    // because it can be changed when going from .r99 to .s00
    if (NumPtr>NextName)
      NumPtr--;

    int CharsToCopy=strlen(NextName)-(NumPtr-NextName);
    int DestPos=strlenw(NextNameW)-CharsToCopy;
    if (DestPos>0)
    {
      CharToWide(NumPtr,NextNameW+DestPos,ASIZE(NextNameW)-DestPos-1);
      NextNameW[ASIZE(NextNameW)-1]=0;
    }
  }

#if !defined(SFX_MODULE) && !defined(RARDLL)
  bool RecoveryDone=false;
#endif
  bool FailedOpen=false,OldSchemeTested=false;

  while (!Arc.Open(NextName,NextNameW))
  {
    if (!OldSchemeTested)
    {
      char AltNextName[NM];
      strcpy(AltNextName,Arc.FileName);
      NextVolumeName(AltNextName,true);
      OldSchemeTested=true;
      if (Arc.Open(AltNextName))
      {
        strcpy(NextName,AltNextName);
        *NextNameW=0;
        break;
      }
    }
#ifdef RARDLL
    if (Cmd->Callback==NULL && Cmd->ChangeVolProc==NULL ||
        Cmd->Callback!=NULL && Cmd->Callback(UCM_CHANGEVOLUME,Cmd->UserData,(LONG)NextName,RAR_VOL_ASK)==-1)
    {
      Cmd->DllError=ERAR_EOPEN;
      FailedOpen=true;
      break;
    }
    if (Cmd->ChangeVolProc!=NULL)
    {
#if defined(_WIN_32) && !defined(_MSC_VER) && !defined(__MINGW32__)
      _EBX=_ESP;
#endif
      int RetCode=Cmd->ChangeVolProc(NextName,RAR_VOL_ASK);
#if defined(_WIN_32) && !defined(_MSC_VER) && !defined(__MINGW32__)
      _ESP=_EBX;
#endif
      if (RetCode==0)
      {
        Cmd->DllError=ERAR_EOPEN;
        FailedOpen=true;
        break;
      }
    }
#else // RARDLL

#if !defined(SFX_MODULE) && !defined(_WIN_CE)
    if (!RecoveryDone)
    {
      RecVolumes RecVol;
      RecVol.Restore(Cmd,Arc.FileName,Arc.FileNameW,true);
      RecoveryDone=true;
      continue;
    }
#endif

#ifndef GUI
    if (!Cmd->VolumePause && !IsRemovable(NextName))
    {
      FailedOpen=true;
      break;
    }
#endif
#ifndef SILENT
    if (Cmd->AllYes || !AskNextVol(NextName))
#endif
    {
      FailedOpen=true;
      break;
    }
#endif // RARDLL
    *NextNameW=0;
  }
  if (FailedOpen)
  {
#if !defined(SILENT) && !defined(_WIN_CE)
      Log(Arc.FileName,St(MAbsNextVol),NextName);
#endif
    Arc.Open(Arc.FileName,Arc.FileNameW);
    Arc.Seek(PosBeforeClose,SEEK_SET);
    return(false);
  }
  Arc.CheckArc(true);
#ifdef RARDLL
  if (Cmd->Callback!=NULL &&
      Cmd->Callback(UCM_CHANGEVOLUME,Cmd->UserData,(LONG)NextName,RAR_VOL_NOTIFY)==-1)
    return(false);
  if (Cmd->ChangeVolProc!=NULL)
  {
#if defined(_WIN_32) && !defined(_MSC_VER) && !defined(__MINGW32__)
    _EBX=_ESP;
#endif
    int RetCode=Cmd->ChangeVolProc(NextName,RAR_VOL_NOTIFY);
#if defined(_WIN_32) && !defined(_MSC_VER) && !defined(__MINGW32__)
    _ESP=_EBX;
#endif
    if (RetCode==0)
      return(false);
  }
#endif

  if (Command=='T' || Command=='X' || Command=='E')
    mprintf(St(Command=='T' ? MTestVol:MExtrVol),Arc.FileName);
  if (SplitHeader)
    Arc.SearchBlock(HeaderType);
  else
    Arc.ReadHeader();
  if (Arc.GetHeaderType()==FILE_HEAD)
  {
    Arc.ConvertAttributes();
    Arc.Seek(Arc.NextBlockPos-Arc.NewLhd.FullPackSize,SEEK_SET);
  }
#ifndef GUI
  if (ShowFileName)
  {
    char OutName[NM];
    IntToExt(Arc.NewLhd.FileName,OutName);
#ifdef UNICODE_SUPPORTED
    bool WideName=(Arc.NewLhd.Flags & LHD_UNICODE) && UnicodeEnabled();
    if (WideName)
    {
      wchar NameW[NM];
      ConvertPath(Arc.NewLhd.FileNameW,NameW);
      char Name[NM];
      if (WideToChar(NameW,Name) && IsNameUsable(Name))
        strcpy(OutName,Name);
    }
#endif
    mprintf(St(MExtrPoints),OutName);
    if (!Cmd->DisablePercentage)
      mprintf("     ");
  }
#endif
  if (DataIO!=NULL)
  {
    if (HeaderType==ENDARC_HEAD)
      DataIO->UnpVolume=false;
    else
    {
      DataIO->UnpVolume=(hd->Flags & LHD_SPLIT_AFTER);
      DataIO->SetPackedSizeToRead(hd->FullPackSize);
    }
#ifdef SFX_MODULE
    DataIO->UnpArcSize=Arc.FileLength();
    DataIO->CurUnpRead=0;
#endif
    DataIO->PackedCRC=0xffffffff;
//    DataIO->SetFiles(&Arc,NULL);
  }
  return(true);
}






#ifndef SILENT
bool AskNextVol(char *ArcName)
{
  eprintf(St(MAskNextVol),ArcName);
  if (Ask(St(MContinueQuit))==2)
    return(false);
  return(true);
}
#endif
