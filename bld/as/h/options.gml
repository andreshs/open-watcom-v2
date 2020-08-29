:cmt
:cmt !!!!! This file must be in UTF-8 encoding !!!!!!!
:cmt

:cmt
:cmt GML Macros used:
:cmt
:cmt    :chain. <option> <usage text>           options that start with <option>
:cmt                                            can be chained together i.e.,
:cmt                                            -oa -ox -ot => -oaxt
:cmt    :option. <option> <synonym> ...         define an option
:cmt    :target. <arch1> <arch2> ...            valid for these architectures
:cmt    :ntarget. <arch1> <arch2> ...           not valid for these architectures
:cmt    :immediate. <fn>                        <fn> is called when option parsed
:cmt    :code. <source-code>                    <source-code> is executed when option parsed
:cmt    :enumerate. <field> [<value>]           option is one value in <name> enumeration
:cmt    :number. [<fn>] [<default>]             =<n> allowed; call <fn> to check
:cmt    :id. [<fn>]                             =<id> req'd; call <fn> to check
:cmt    :char.[<fn>]                            =<char> req'd; call <fn> to check
:cmt    :file.                                  =<file> req'd
:cmt    :path.                                  =<path> req'd
:cmt    :special. <fn> [<arg_usage_text>]       call <fn> to parse option
:cmt    :optional.                              value is optional
:cmt    :internal.                              option is undocumented
:cmt    :prefix.                                prefix of a :special. option
:cmt    :usagegrp. <option> <usage text>        group of options that start with <option>
:cmt                                            they are chained together in usage text only
:cmt    :usage. <text>                          English usage text
:cmt    :jusage. <text>                         Japanese usage text
:cmt    :title.                                 English title usage text
:cmt    :jtitle.                                Japanese title usage text
:cmt    :titleu.                                English title usage text for QNX resource file
:cmt    :jtitleu.                               Japanese title usage text for QNX resource file
:cmt    :page.                                  text for paging usage message
:cmt    :nochain.                               option isn't chained with other options
:cmt    :timestamp.                             kludge to record "when" an option
:cmt                                            is set so that dependencies
:cmt                                            between options can be simulated
:cmt
:cmt Global macros
:cmt
:cmt    :noequal.                               args can't have option '='
:cmt    :argequal. <char>                       args use <char> instead of '='
:cmt

:cmt    where:
:cmt        <arch>:     i86, 386, axp, any, dbg, qnx, ppc, linux, sparc, haiku

:cmt    Translations are required for the :jtitle. and :jusage. tags
:cmt    if there is no text associated with the tag.


:title. Usage: wasaxp {options} {asm_files}
:jtitle.
:target. axp

:title. Usage: wasppc {options} {asm_files}
:jtitle.
:target. ppc

:title. Usage: wasmips {options} {asm_files}
:jtitle.
:target. mps

:title. Options:
:jtitle.
:target. any
:title.  .         ( /option is also accepted )
:jtitle. .         ( /optionも使用できます )
:target. any
:ntarget. bsd linux osx qnx haiku

:page. (Press return to continue)
:jusage. (リターンを押すと，続行します)

:option. d
:target. any
:nochain.
:special. scanDefine <name>[=text]
:usage. define text macro
:jusage.

:option. e
:target. any
:number.
:usage. set error limit number
:jusage.

:option. fo
:target. any
:file.
:usage. set output filename (applies to the first asm_file)
:jusage.

:option. h ?
:target. any
:nochain.
:usage. print this message
:jusage.

:option. i
:target. any
:path.
:usage. set include path
:jusage.

:chain. o Output object file format
:jusage.

:option. oc
:target. any
:path.
:usage. COFF object file format
:jusage.

:option. oe
:target. any
:path.
:usage. ELF object file format
:jusage.

:option. q zq
:target. any
:usage. operate quietly
:jusage.

:option. we
:target. any
:usage. treat all warnings as errors
:jusage.

:chain. v Debug verbose output
:jusage.

:option. vi
:target. any
:internal.
:usage. view instruction
:jusage.

:option. vl
:target. any
:internal.
:usage. view lex buffer
:jusage.

:option. vp
:target. any
:internal.
:usage. view parse
:jusage.

:option. vs
:target. any
:internal.
:usage. view symbols
:jusage.

:option. vt
:target. any
:internal.
:usage. view ins table
:jusage.
