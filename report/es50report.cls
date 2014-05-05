%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
% ES50 Final Project Report Class
%
% Michael Traver '14
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

\NeedsTeXFormat{LaTeX2e}
\ProvidesClass{es50report}[2014/05/05 ES50 Final Project Report Format]

\LoadClass[12pt,letterpaper]{article}

\usepackage[left=1.25in, right=1.25in, top=1in, bottom=1in]{geometry}

\frenchspacing

\usepackage{fancyhdr}
\setlength{\headheight}{15.2pt}
\fancyhf{}
\pagestyle{fancy}

% remove decorative lines from header and footer
\renewcommand{\headrulewidth}{0.0pt}
\renewcommand{\footrulewidth}{0.0pt}

% clear header and set footer
\fancyhead{}
\fancyfoot[L]{ES50 Project 2014}
\fancyfoot[C]{Page \thepage}

\usepackage{datetime}
\newdateformat{mydate}{\monthname[\THEMONTH] \THEYEAR}

% \usepackage{sectsty}
% \sectionfont{\MakeUppercase}

% commands for setting the title page components
\let\@title\relax
\def\title#1{\def\@title{#1}}

\let\@description\relax
\def\description#1{\def\@description{#1}}

\let\@authors\relax
\def\authors#1{\def\@authors{#1}}

% typesets the title page
\newcommand{\maketitlepage}{%
    \begin{center}
        {\huge \textbf{\@title}} \\
        \bigskip\bigskip\bigskip

        School of Engineering and Applied Sciences \\
        Harvard University \\
        33 Oxford Street \\
        Cambridge, MA \\
        \bigskip\bigskip\bigskip\bigskip\bigskip
        
        \textbf{Description:} \@description \\
        \bigskip

        \textbf{By:} \@authors \\
        \bigskip\bigskip

        \textbf{Class:} Engineering Sciences 50 \\
        \textbf{Instructors:} Marko Lon\v{c}ar and Evelyn Hu
        \bigskip\bigskip\bigskip\bigskip

        \mydate\today \\
        \bigskip\bigskip\bigskip
    \end{center}
}
