" Modded version of "pablo" color scheme: newer vim versions ignore custom color overrides outside scheme files (vim bug #542).
" The workaround is moving all overrides into the scheme file, so here it is.
"
" - Richard Nusser, 2017-04-26
"

hi clear
set background=dark
if exists("syntax_on")
  syntax reset
endif
let g:colors_name="pablo_rnfix"

highlight Comment    ctermfg=8                          guifg=#808080
highlight Constant   ctermfg=14              cterm=none guifg=#00ffff               gui=none
highlight Identifier ctermfg=6                          guifg=#00c0c0
highlight Statement  ctermfg=3               cterm=bold guifg=#c0c000               gui=bold
highlight PreProc    ctermfg=10                         guifg=#00ff00
highlight Type       ctermfg=2                          guifg=#00c000
highlight Special    ctermfg=12                         guifg=#0000ff
highlight Error                  ctermbg=9                            guibg=#ff0000
highlight Todo       ctermfg=4   ctermbg=3              guifg=#000080 guibg=#c0c000
highlight Directory  ctermfg=2                          guifg=#00c000
highlight StatusLine ctermfg=11  ctermbg=12  cterm=none guifg=#ffff00 guibg=#0000ff gui=none
highlight Normal                                        guifg=#ffffff guibg=#000000
highlight Search                 ctermbg=3                            guibg=#c0c000

highlight StatusLine ctermfg=253 ctermbg=236            guifg=#d0d0d0 guibg=#303030
highlight TAG_DEBUG  ctermfg=255 ctermbg=242
highlight Search                 ctermbg=192
highlight Todo                   ctermbg=202
