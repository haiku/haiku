Program uuencode;

  CONST header = 'begin';
        trailer = 'end';
        defaultMode = '644';
        defaultExtension = '.uue';
        offset = 32;
        charsPerLine = 60;
        bytesPerHunk = 3;
        sixBitMask = $3F;

  TYPE string80 = string[80];

  VAR infile: file of byte;
      outfile: text;
      infilename, outfilename, mode: string80;
      lineLength, numbytes, bytesInLine: integer;
      line: array [0..59] of char;
      hunk: array [0..2] of byte;
      chars: array [0..3] of byte;
      size,remaining :real;

{  procedure debug;

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
      for i := 0 to 2 do writebin(hunk[i]);
      writeln;
      for i := 0 to 3 do writebin(chars[i]);
      writeln;
      for i := 0 to 3 do writebin(chars[i] and sixBitMask);
      writeln
    end;  }

  procedure Abort (message: string80);

    begin {abort}
      writeln(message);
      close(infile);
      close(outfile);
      halt
    end; {abort}

  procedure Init;

    procedure GetFiles;

      VAR i: integer;
          temp: string80;
          ch: char;

      begin {GetFiles}
        if ParamCount < 1 then abort ('No input file specified.');
        infilename := ParamStr(1);
        {$I-}
        assign (infile, infilename);
        reset (infile);
        {$i+}
        if IOResult > 0 then abort (concat ('Can''t open file ', infilename));
        size:=FileSize(infile);
        if size < 0 then size:=size+65536.0;
        remaining:=size;
        write('Uuencoding file ', infilename);

        i := pos('.', infilename);
        if i = 0
          then outfilename := infilename
          else outfilename := copy (infilename, 1, pred(i));
        mode := defaultMode;
        if ParamCount > 1 then
          for i := 2 to ParamCount do
            begin
              temp := Paramstr(i);
              if temp[1] in ['0'..'9']
                then mode := temp
                else outfilename := temp
            end;
        if pos ('.', outfilename) = 0
          then outfilename := concat(outfilename, defaultExtension);
        assign (outfile, outfilename);
        writeln (' to file ', outfilename, '.');

        {$i-}
        reset(outfile);
        {$i+}
        if IOresult = 0 then
          begin
            Write ('Overwrite current ', outfilename, '? [Y/N] ');
            repeat
              read (kbd, ch);
              ch := Upcase(ch)
            until ch in ['Y', 'N'];
            writeln (ch);
            if ch = 'N' then abort(concat (outfilename, ' not overwritten.'))
          end;
        close(outfile);

        {$i-}
        rewrite(outfile);
        {$i+}
        if ioresult > 0 then abort(concat('Can''t open ', outfilename));
      end; {getfiles}

    begin {Init}
      GetFiles;
      bytesInLine := 0;
      lineLength := 0;
      numbytes := 0;
      writeln (outfile, header, ' ', mode, ' ', infilename);
    end; {init}

  procedure FlushLine;

    VAR i: integer;

    procedure writeout(ch: char);

      begin {writeout}
        if ch = ' ' then write(outfile, '`')
                    else write(outfile, ch)
      end; {writeout}

    begin {FlushLine}
      {write ('.');}
      write('bytes remaining: ',remaining:7:0,' (',
            remaining/size*100.0:3:0,'%)',chr(13));
      writeout(chr(bytesInLine + offset));
      for i := 0 to pred(lineLength) do
        writeout(line[i]);
      writeln (outfile);
      lineLength := 0;
      bytesInLine := 0
    end; {FlushLine}

  procedure FlushHunk;

    VAR i: integer;

    begin {FlushHunk}
      if lineLength = charsPerLine then FlushLine;
      chars[0] := hunk[0] shr 2;
      chars[1] := (hunk[0] shl 4) + (hunk[1] shr 4);
      chars[2] := (hunk[1] shl 2) + (hunk[2] shr 6);
      chars[3] := hunk[2] and sixBitMask;
      {debug;}
      for i := 0 to 3 do
        begin
          line[lineLength] := chr((chars[i] and sixBitMask) + offset);
          {write(line[linelength]:2);}
          lineLength := succ(lineLength)
        end;
      {writeln;}
      bytesInLine := bytesInLine + numbytes;
      numbytes := 0
    end; {FlushHunk}

  procedure encode1;

    begin {encode1};
      if numbytes = bytesperhunk then flushhunk;
      read (infile, hunk[numbytes]);
      remaining:=remaining-1;
      numbytes := succ(numbytes)
    end; {encode1}

  procedure terminate;

    begin {terminate}
      if numbytes > 0 then flushhunk;
      if lineLength > 0
        then
          begin
            flushLine;
            flushLine;
          end
        else flushline;
      writeln (outfile, trailer);
      close (outfile);
      close (infile);
    end; {terminate}


  begin {uuencode}
    init;
    while not eof (infile) do encode1;
    terminate;
    writeln;
  end. {uuencode}
