program uudecode;

  CONST defaultSuffix = '.uue';
        offset = 32;

  TYPE string80 = string[80];

  VAR infile: text;
      fi    : file of byte;
      outfile: file of byte;
      lineNum: integer;
      line: string80;
      size,remaining :real;

  procedure Abort(message: string80);

    begin {abort}
      writeln;
      if lineNum > 0 then write('Line ', lineNum, ': ');
      writeln(message);
      halt
    end; {Abort}

  procedure NextLine(var s: string80);

    begin {NextLine}
      LineNum := succ(LineNum);
      {write('.');}
      readln(infile, s);
      remaining:=remaining-length(s)-2;  {-2 is for CR/LF}
      write('bytes remaining: ',remaining:7:0,' (',
            remaining/size*100.0:3:0,'%)',chr(13));
    end; {NextLine}

  procedure Init;

    procedure GetInFile;

      VAR infilename: string80;

      begin {GetInFile}
        if ParamCount = 0 then abort ('Usage: uudecode <filename>');
        infilename := ParamStr(1);
        if pos('.', infilename) = 0
          then infilename := concat(infilename, defaultSuffix);
        assign(infile, infilename);
        {$i-}
        reset(infile);
        {$i+}
        if IOresult > 0 then abort (concat('Can''t open ', infilename));
        writeln ('Decoding ', infilename);
        assign(fi,infilename); reset(fi);
        size:=FileSize(fi); close(fi);
        if size < 0 then size:=size+65536.0;
        remaining:=size;
      end; {GetInFile}

    procedure GetOutFile;

      var header, mode, outfilename: string80;
          ch: char;

      procedure ParseHeader;

        VAR index: integer;

        Procedure NextWord(var word:string80; var index: integer);

          begin {nextword}
            word := '';
            while header[index] = ' ' do
              begin
                index := succ(index);
                if index > length(header) then abort ('Incomplete header')
              end;
            while header[index] <> ' ' do
              begin
                word := concat(word, header[index]);
                index := succ(index)
              end
          end; {NextWord}

        begin {ParseHeader}
          header := concat(header, ' ');
          index := 7;
          NextWord(mode, index);
          NextWord(outfilename, index)
        end; {ParseHeader}

      begin {GetOutFile}
        if eof(infile) then abort('Nothing to decode.');
        NextLine (header);
        while not ((copy(header, 1, 6) = 'begin ') or eof(infile)) do
          NextLine(header);
        writeln;
        if eof(infile) then abort('Nothing to decode.');
        ParseHeader;
        assign(outfile, outfilename);
        writeln ('Destination is ', outfilename);
        {$i-}
        reset(outfile);
        {$i+}
        if IOresult = 0 then
          begin
            write ('Overwrite current ', outfilename, '? [Y/N] ');
            repeat
              read (kbd, ch);
              ch := UpCase(ch)
            until ch in ['Y', 'N'];
            writeln(ch);
            if ch = 'N' then abort ('Overwrite cancelled.')
          end;
        rewrite (outfile);
      end; {GetOutFile}

    begin {init}
      lineNum := 0;
      GetInFile;
      GetOutFile;
    end; { init}

  Function CheckLine: boolean;

    begin {CheckLine}
      if line = '' then abort ('Blank line in file');
      CheckLine := not (line[1] in [' ', '`'])
    end; {CheckLine}


  procedure DecodeLine;

    VAR lineIndex, byteNum, count, i: integer;
        chars: array [0..3] of byte;
        hunk: array [0..2] of byte;

{    procedure debug;

      var i: integer;

      procedure writebin(x: byte);

        var i: integer;

        begin
          for i := 1 to 8 do
            begin
              write ((x and $80) shr 7);
              x := x shl 1
            end;
          write (' ')
        end;

      begin
        writeln;
        for i := 0 to 3 do writebin(chars[i]);
        writeln;
        for i := 0 to 2 do writebin(hunk[i]);
        writeln
      end;      }

    function nextch: char;

      begin {nextch}
        lineIndex := succ(lineIndex);
        if lineIndex > length(line) then abort('Line too short.');
        if not (line[lineindex] in [' '..'`'])
          then abort('Illegal character in line.');
{        write(line[lineindex]:2);}
        if line[lineindex] = '`' then nextch := ' '
                                 else nextch := line[lineIndex]
      end; {nextch}

    procedure DecodeByte;

      procedure GetNextHunk;

        VAR i: integer;

        begin {GetNextHunk}
          for i := 0 to 3 do chars[i] := ord(nextch) - offset;
          hunk[0] := (chars[0] shl 2) + (chars[1] shr 4);
          hunk[1] := (chars[1] shl 4) + (chars[2] shr 2);
          hunk[2] := (chars[2] shl 6) + chars[3];
          byteNum := 0  {;
          debug          }
        end; {GetNextHunk}

      begin {DecodeByte}
        if byteNum = 3 then GetNextHunk;
        write (outfile, hunk[byteNum]);
        {writeln(bytenum, ' ', hunk[byteNum]);}
        byteNum := succ(byteNum)
      end; {DecodeByte}

    begin {DecodeLine}
      lineIndex := 0;
      byteNum := 3;
      count := (ord(nextch) - offset);
      for i := 1 to count do DecodeByte
    end; {DecodeLine}

  procedure terminate;

    var trailer: string80;

    begin {terminate}
      if eof(infile) then abort ('Abnormal end.');
      NextLine (trailer);
      if length (trailer) < 3 then abort ('Abnormal end.');
      if copy (trailer, 1, 3) <> 'end' then abort ('Abnormal end.');
      close (infile);
      close (outfile)
    end;

  begin {uudecode}
    init;
    NextLine(line);
    while CheckLine do
      begin
        DecodeLine;
        NextLine(line)
      end;
    terminate
  end.
