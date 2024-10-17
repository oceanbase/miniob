window.MathJax = {
    tex: {
      inlineMath: [ ["\\(","\\)"], ['$', '$'] ],
      displayMath: [ ["\\[","\\]"], ['$$', '$$'] ],
      tags: 'ams'
    },
    options: {
      ignoreHtmlClass: ".*|",
      processHtmlClass: "arithmatex"
    }
  };