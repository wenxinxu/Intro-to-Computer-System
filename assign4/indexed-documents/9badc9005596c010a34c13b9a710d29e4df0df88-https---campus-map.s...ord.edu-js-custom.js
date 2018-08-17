// JavaScript Document


/* 
 * http://www.merriampark.com/ld.htm, 
 * http://www.mgilleland.com/ld/ldjavascript.htm, Damerau�Levenshtein distance (Wikipedia)
 */
var levDist = function(s, t) 
              {
                  var d = []; //2d matrix

                  // Step 1
                  var n = s.length;
                  var m = t.length;

                  if (n == 0) 
                      return m;
                  if (m == 0) 
                      return n;

                  //Create an array of arrays in javascript (a descending loop is quicker)
                  for (var i = n; i >= 0; i--) 
                      d[i] = [];

                  // Step 2
                  for (var i = n; i >= 0; i--) 
                      d[i][0] = i;
                  for (var j = m; j >= 0; j--) 
                      d[0][j] = j;

                  // Step 3
                  for (var i = 1; i <= n; i++) 
                  {
                      var s_i = s.charAt(i - 1);
                      // Step 4
                      for (var j = 1; j <= m; j++) 
                      {
                          //Check the jagged ld total so far
                          if (i == j && d[i][j] > 4) 
                              return n;

                          var t_j = t.charAt(j - 1);
                          var cost = (s_i == t_j) ? 0 : 1; // Step 5

                          //Calculate the minimum
                          var mi = d[i - 1][j] + 1;
                          var b = d[i][j - 1] + 1;
                          var c = d[i - 1][j - 1] + cost;

                          if (b < mi) 
                              mi = b;
                          if (c < mi) 
                              mi = c;

                          d[i][j] = mi; // Step 6

                          // Damerau transposition
                          if (i > 1 && j > 1 && s_i == t.charAt(j - 2) && s.charAt(i - 2) == t_j) 
                              d[i][j] = Math.min(d[i][j], d[i - 2][j - 2] + cost);
                      }
                  }

                  // Step 7
                  return d[n][m];
              }

var dbg_1 = 401;
var curr_matches;

/*
 *  phrase = words to search
 *  search = search term
 */
function ld_chk(phrase, search, p_min_len)
{
    var min_dist = 20;
    var perf_cnt = 0;
    var ld, min_len, pos;
    var all_num_match;
    var curr_match = "";
    // var words = phrase.toLowerCase().split(" ");
    // var terms = search.toLowerCase().split(" ");
    var words = [];
    words[0] = phrase;
    var terms = [];
    terms[0] = search.toLowerCase();
    if ( search.length > 11 && phrase.indexOf(search.substr(0, search.length - 5)) != -1 )
    {
        // A very good partial match.
        min_dist = 0;
        search = curr_match = search.substr(0, search.length - 3); 
        perf_cnt += search.replace(/\w/g, "").length;
    }
    else
    {
        for(i=0; i < words.length; i++)
        {
             if ( p_min_len > words[i].length )
                min_len = words[i].length > 2 ? words[i].length - 2 : 1;
             else
                min_len = p_min_len;
             for(j=0; j < terms.length; j++)
             {
                 // Find the position in the location name to be searched that contains the first or first two (closer match) chars.
                 //   Skip search terms and "words" in the building name  that are only one character.
                 if ( words[i].indexOf(terms[j].substr(0,2)) == -1 )
                    pos = 1
                 else
                    //  The match does not count if there is a whitespace in the location and the search term does not have whitespace.
                    if ( terms[j].indexOf(" ") == -1 )
                        if ( words[i].substr(words[i].indexOf(terms[j].substr(0,2)), terms[j].length).indexOf(" ") == -1 )
                            pos = 2
                        else
                            pos = 1
                     else
                        pos = 2
                 ld = levDist(words[i].substr(words[i].indexOf(terms[j].substr(0,pos)), terms[j].length), terms[j]);
                 // if ( words[i].substr(0, 4) == "1 pe" )
                 //    console.log( "1. " + words[i] + "-" + ld + "-" + pos);
                 // Skip if the distance indicates that the entire word is replaced.  Not a match.
                 if ( min_dist > ld && ld < words[i].length -2 )
                 {
                     // Only match if all of a _numeric_ search term's chars exist in this word.
                     all_num_match = 1;
                     for(k=0; k < words[i].length; k++)
                     {
                        if ( words[i].indexOf(terms[j].substr(k, 1)) == -1 )
                        {
                            all_num_match = 0;
                            break;
                        }
                     }
                     if ( terms[j].replace(/[0-9-]/g, "") == "" )
                     {
                         // Numeric must match within one substitution.
                         if ( ld < 2 && all_num_match == 1 )
                         {
                             min_dist = ld;
                             curr_match = words[i].substr(words[i].indexOf(terms[j].substr(0,pos)), terms[j].length); 
                         }
                     }
                     else
                     {
                         // if ( words[i].substr(0, 4) == "1 pe" )
                         //     console.log( "2. " + words[i] + "-" + ld + "-" + pos);
                         // Give more weight if the match occurs at the start of the location name or the start of a word in the name.
                         //   A lower ld is better.
                         switch( words[i].indexOf(terms[j]) )
                         {
                             case -1:
                                break;
                             case 0:
                                ld -= 2;
                                break;
                             default:
                                if ( words[i].substr(words[i].indexOf(terms[j]) -1, 1) == " " )
                                    ld--;
                         }
                         // if ( words[i].substr(0, 4) == "1 pe" )
                         //     console.log( "3. " + words[i] + "-" + ld + "-" + pos);
                         min_dist = ld;
                         curr_match = words[i].substr(words[i].indexOf(terms[j].substr(0,pos)), terms[j].length);
                     }
                 }
                 // Single word perfect match!
                 if ( ld < 1 )
                 {
                     perf_cnt += (terms.length -j + 1) * 3;           // Give added weight to the front words.
                     // console.log(phrase + " -- " + terms[j] + " -- " + search + " -- " + perf_cnt + " -- " + min_dist + " -- " + ld + "-" + pos);
                     break;
                 }
                 if ( dbg_1 < 400  && phrase.toLowerCase().indexOf("bonair") > 0 )
                 {
                     console.log(dbg_1 + ": " + phrase + " -- " + terms[j] + "-" + ld + "-" + perf_cnt);
                     dbg_1++;
                 }
             }
        }
    }
    if ( curr_match.length > 0 && min_dist < min_len )
    {
        var exists = 0;
        for(var i=0; i < curr_matches.length; i++)
        {
            if ( curr_matches[i][0] == curr_match )
            {
                exists = 1;
                break;
            }
        }
        if ( exists == 0 && curr_match.length == search.length )
        {
            var idx = curr_matches.length;
            curr_matches[idx] = [curr_match, ld];
            // console.log(curr_match + "--" + curr_match.length);
        }
    }
    // if ( min_dist - perf_cnt  < 2 )
    //    console.log(phrase + "--" + search + "--" + min_dist + "-" + perf_cnt);
    return min_dist - perf_cnt;
}

function srch()
{
    var list;
    var encode = function(item)
                 {
                     return item.replace(/%/g, "%25").replace(/&/g, "%26").replace(/#/g, "%23");
                 }
    this.get = function()
               {
                   return list;
               }
    this.clr = function(item)
               {
                   list = "";
               }
    this.set = function(item)
               {
                   list = encode(item);
               }
    this.append = function(item)
                  {
                      // Most browsers fail if the url is too long.
                      if ( list.length < 2000 )
                          list += "~" + encode(item);
                  }
    this.is_empty = function(item)
                    {
                       if ( list.length == 0 )
                           return true;
                       return false;
                    }
}

var srch_lst = new srch();

var moar_info = 0;
function close_more_info()
{
    $("#search_address").popover('hide');
    $("#search_address").popover('destroy');
    moar_info = 0;
}



function more_info()
{
    var search = $("#search_address").val();
    var msg = "";
    var msg_pre = "<a id='moar_info' class='highlight-suggestion' href='#' onclick='javascript: show_more_info();' style='text-decoration: underline'>"; 
    var msg_post = "</a>";
    var msg_size = 0;
    //var link_text = "Explore pane <img width='35' height='35' alt='explore' src='images/explore_1black.png'>";
	 var link_text = "";
    if ( search.substr(0,4).toLowerCase() == "park" )
        msg = "<span class='glyphicon glyphicon-scale'></span>" + msg_pre + "Explore More Parking" + link_text + msg_post;
    if ( search.substr(0,3).toLowerCase() == "eat" )
        msg = "<span class='glyphicon glyphicon-cutlery'></span>" + msg_pre + "Explore More Eateries" + link_text + msg_post;
    if ( msg == "" )
    {
        close_more_info();
    }
    else
    {
        if ( moar_info == 0 )
        {
			return msg;
        }
    }
	
}

function show_more_info()
{
    var panel = $('.slide-panel'); 
    var search = $("#search_address").val();
    var content = "";
    var catid = 0;
    if ( panel.css("margin-left") != "0px" )
        panel.addClass('visible').animate({'margin-left':'0px'});
    $('#menuPanel').show();
    $('#searchPanel').hide();
    $('#directionPanel').hide();

    close_more_info();

    $(".panel-default").each(function()
                             {
                                $(this).children().each(function()
                                                        {
                                                            if ( typeof $(this).attr("id") == "undefined" )
                                                                content = $(this).html();

                                                            if ( search.substr(0,3).toLowerCase() == "eat" )
                                                                catid = 20;
                                                            if ( search.substr(0,4).toLowerCase() == "park" )
                                                                catid = 30;

                                                            if ( typeof $(this).attr("id") == "undefined" )
                                                            {
                                                                if ( $(this).attr("data-target") == "#collapse_" + catid )
                                                                {
                                                                    // Turn the arrow down in the category area.
                                                                    $(this).removeClass("collapsed");
                                                                }
                                                            }
                                                            else
                                                            {
                                                                if ( $(this).attr("id") == "collapse_" + catid )
                                                                {
                                                                    $(this).collapse("show");
                                                                }
                                                                else
                                                                {
                                                                    // Collapse if open.
                                                                    if ( $(this).attr("class").indexOf("in") > 0 )
                                                                    {
                                                                       $(this).collapse("hide");
                                                                   }
                                                                }
                                                            }
                                                        });
                             });
}

$(function() 
  {
    var min_len = 3;
    var dbg_2 = 401;
    var dbg_3 = 401;
    if ($('#search_address').length)
    {
    $("#search_address").autocomplete({ source: function(req, resp)
                                                {
                                                    var search = req.term;
                                                    srch_lst.clr();
                                                    curr_matches = [["n/a", 1000]];
                                                    $("#dir_driv_info").html("");
													
                                                    // Use this in place of the minLength option to permit the above messages.
                                                    if ( search.length >= min_len && search.replace(/ /g, "").length > 1 )
                                                    {
                                                        var matches = bldgs.filter(function(item) 
                                                                                   {
                                                                                      var found = 0;
                                                                                      var match_dist = 2;
                                                                                      /*
                                                                                      var match_dist = search.length-2 > 2 ? search.length-2 : 2;
                                                                                      if ( match_dist > 2 )
                                                                                           match_dist = 2;
                                                                                       */
                                                                                      if ( search.replace(/ /g, "").length < 3 )
                                                                                          match_dist = 1;
                                                                                      var ld = ld_chk(item, search, min_len);
                                                                                      if ( dbg_2 < 400 && search.length > min_len )
                                                                                      {
                                                                                          console.log("Debug: " + item + "-" + ld + "-" + match_dist);
                                                                                          dbg_2++;
                                                                                      }
                                                                                      if ( ld < match_dist )
                                                                                      {
                                                                                         found = 1;
                                                                                         if ( srch_lst.is_empty() )
                                                                                             srch_lst.set(item);
                                                                                         else
                                                                                             srch_lst.append(item);
                                                                                         if ( dbg_3 < 400 )
                                                                                         {
                                                                                             console.log("Found: " + item + "-" + ld);
                                                                                             dbg_3++;
                                                                                         }
                                                                                      }
                                                                                      if ( found == 0 )
                                                                                          return false;
                                                                                      return true;
                                                                                   }).sort(function (a, b) 
                                                                                           {
                                                                                                var score_a = ld_chk(a, search);
                                                                                                var score_b = ld_chk(b, search);
                                                                                                return score_a < score_b ? -1 : score_a === score_b ? 0 : 1;
                                                                                           });
                                                        resp(matches.slice(0, 40));   // Limit search results.
                                                    }
                                                },
                                       open: function(event, ui)
                                             {
                                                 $("#pm_btn1").attr("disabled", false);
                                                 $(".ui-autocomplete").scrollTop(0); 
												 
												  /************************************************
												   * TEST to add the bubble content into the list *
												   ************************************************/
												 if (more_info()){
													$(".ui-autocomplete").prepend('<li><p>Suggestion:</p><a href="#" class="highlight">'+more_info()+'</a></li>');

											 }
												 
                                                 return false;
                                             },
                                       select: function(event, ui)
                                               {
                                                   // Exact match.
                                                   bldg_srch(ui.item.value, 1);
                                                   return false;
                                               },
                                      }).data('autocomplete')._renderItem = function(ul, item)
                                                                            {
                                                                                  var curr_match = "";
                                                                                  var search = $("#search_address").val();
                                                                                  var min_dist = 100;
                                                                                  var re;
                                                                                  for(i=0; i < curr_matches.length; i++)
                                                                                  {
                                                                                    if ( item.label.toLowerCase().indexOf(curr_matches[i][0]) != -1 )
                                                                                    {
                                                                                        if ( min_dist > curr_matches[i][1] )
                                                                                        {
                                                                                            curr_match = curr_matches[i][0].replace(/[^A-Za-z0-9]/, "");
                                                                                            min_dist = curr_matches[i][1];
                                                                                            // console.log("here- " + i + "--" + min_dist + "--" + curr_matches[i][0] + "--" + curr_matches[i][1]);
                                                                                        }
                                                                                        if ( curr_match == search )
                                                                                            break;
                                                                                    }
                                                                                  }
                                                                                  curr_match = "(" + curr_match + ")"; 
                                                                                  re = new RegExp(curr_match, "ig");
                                                                                  return $( "<li></li>" ).data("item.autocomplete", item )
                                                                                                         .append( "<a>" + item.label.replace(re, '<b>$1</b>') + "</a>")
				                                                                                         .appendTo( ul );

                                                                             }
    $("#search_address").keyup(function(evt)
                               {
                                   if ( evt.which == 13 )
                                   {
                                       $(".ui-menu").hide();
									   $(this).autocomplete("close");
                                   }
                                   more_info();
                               });

    $(".btn_default").click(function(evt)
                            {
                                $(".ui-autocomplete").hide();
                            });

    $(document).click(function(evt) 
                      {
                          var target = $(evt.target);
                          try
                          {
                              if ( ! target.attr('class').match(/^moar_info/) && target.parents('#moar_info').length == 0 )
                                  close_more_info();
                              $("#maplink").hide();
                          }
                          catch(err)
                          {
                              close_more_info();
                          };
                      });
  }
  });
