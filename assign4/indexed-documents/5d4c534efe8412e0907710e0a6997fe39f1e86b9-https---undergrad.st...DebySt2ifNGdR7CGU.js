(function($) {

    Drupal.behaviors.open_framework = {
        attach: function(context, settings) {
            // Reset iPhone, iPad, and iPod zoom on orientation change to landscape
            var mobile_timer = false;
            if ((navigator.userAgent.match(/iPhone/i)) || (navigator.userAgent.match(/iPad/i)) || (navigator.userAgent.match(/iPod/i))) {
                $('#viewport').attr('content', 'width=device-width,minimum-scale=1.0,maximum-scale=1.0,initial-scale=1.0');
                $(window)
                    .bind('gesturestart', function() {
                        clearTimeout(mobile_timer);
                        $('#viewport').attr('content', 'width=device-width,minimum-scale=1.0,maximum-scale=10.0');
                    })
                    .bind('touchend', function() {
                        clearTimeout(mobile_timer);
                        mobile_timer = setTimeout(function() {
                            $('#viewport').attr('content', 'width=device-width,minimum-scale=1.0,maximum-scale=1.0,initial-scale=1.0');
                        }, 1000);
                    });
            }

            // Header Drupal Search Box
            $('#header [name=search_block_form]')
                .val('Search this site...')
                .focus(function() {
                    $(this).val('');
                });

            // Hide border for image links
            $('a:has(img)').css('border', 'none');

            // Apply the Equal Column Height function below by container
            // instead of page-wide
            equalHeightByContainer = function(className) {
                containerIDs = new Array();
                uncontainedExist = false;

                $(className).each(function() {
                    $el = $(this);
                    parentID = $el.offsetParent().attr('id');
                    if (typeof parentID !== 'undefined') {
                        if ($.inArray(parentID, containerIDs) === -1) {
                            containerIDs.push(parentID);
                        }
                    } else {
                        uncontainedExist = true;
                    }
                });

                if (uncontainedExist) {
                    equalHeight(className);
                }

                $.each(containerIDs, function() {
                    equalHeight('#' + this + ' ' + className);
                });
            }

            // Equal Column Height on load and resize
            // Credit: http://codepen.io/micahgodbolt/pen/FgqLc
            equalHeight = function(classname) {
                var currentTallest = 0,
                    currentRowStart = 0,
                    rowDivs = new Array(),
                    $el,
                    topPosition = 0;
                $(classname).each(function() {
                    $el = $(this);
                    $($el).height('auto')
                    topPosition = $el.position().top;

                    if (currentRowStart != topPosition) {
                        for (currentDiv = 0; currentDiv < rowDivs.length; currentDiv++) {
                            rowDivs[currentDiv].height(currentTallest);
                        }
                        rowDivs.length = 0; // empty the array
                        currentRowStart = topPosition;
                        currentTallest = $el.height();
                        rowDivs.push($el);
                    } else {
                        rowDivs.push($el);
                        currentTallest = (currentTallest < $el.height()) ? ($el.height()) : (currentTallest);
                    }
                    for (currentDiv = 0; currentDiv < rowDivs.length; currentDiv++) {
                        rowDivs[currentDiv].height(currentTallest);
                    }
                });
            }

            $(window).load(function() {
                equalHeightByContainer('.column');
            });

            $(window).resize(function() {
                equalHeightByContainer('.column');
            });


            // Add keyboard focus to .element-focusable elements in webkit browsers.
            $('.element-focusable').on('click', function() {
                $($(this).attr('href')).attr('tabindex', '-1').focus();
            });

            // Add placeholder value support for older browsers
            $('input, textarea').placeholder();

        }
    }

})(jQuery);

//Add legacy IE addEventListener support (http://msdn.microsoft.com/en-us/library/ms536343%28VS.85%29.aspx#1)
if (!window.addEventListener) {
    window.addEventListener = function(type, listener, useCapture) {
        attachEvent('on' + type, function() {
            listener(event)
        });
    }
}
//end legacy support addition

// Hide Address Bar in Mobile View
addEventListener("load", function() {
    setTimeout(hideURLbar, 0);
}, false);

function hideURLbar() {
    if (window.pageYOffset < 1) {
        window.scrollTo(0, 1);
    }
}
;
/*! http://mths.be/placeholder v1.8.7 by @mathias */
(function(f,h,c){var a='placeholder' in h.createElement('input'),d='placeholder' in h.createElement('textarea'),i=c.fn,j;if(a&&d){j=i.placeholder=function(){return this};j.input=j.textarea=true}else{j=i.placeholder=function(){return this.filter((a?'textarea':':input')+'[placeholder]').not('.placeholder').bind('focus.placeholder',b).bind('blur.placeholder',e).trigger('blur.placeholder').end()};j.input=a;j.textarea=d;c(function(){c(h).delegate('form','submit.placeholder',function(){var k=c('.placeholder',this).each(b);setTimeout(function(){k.each(e)},10)})});c(f).bind('unload.placeholder',function(){c('.placeholder').val('')})}function g(l){var k={},m=/^jQuery\d+$/;c.each(l.attributes,function(o,n){if(n.specified&&!m.test(n.name)){k[n.name]=n.value}});return k}function b(){var k=c(this);if(k.val()===k.attr('placeholder')&&k.hasClass('placeholder')){if(k.data('placeholder-password')){k.hide().next().show().focus().attr('id',k.removeAttr('id').data('placeholder-id'))}else{k.val('').removeClass('placeholder')}}}function e(){var o,n=c(this),k=n,m=this.id;if(n.val()===''){if(n.is(':password')){if(!n.data('placeholder-textinput')){try{o=n.clone().attr({type:'text'})}catch(l){o=c('<input>').attr(c.extend(g(this),{type:'text'}))}o.removeAttr('name').data('placeholder-password',true).data('placeholder-id',m).bind('focus.placeholder',b);n.data('placeholder-textinput',o).data('placeholder-id',m).before(o)}n=n.removeAttr('id').hide().prev().attr('id',m).show()}n.addClass('placeholder').val(n.attr('placeholder'))}else{n.removeClass('placeholder')}}}(this,document,jQuery));;
(function ($) {

  Drupal.behaviors.open_framework = {
    attach: function (context, settings) {
		
			// Bootstrap Dropdown Menu
			$('#main-menu ul > li:has(.active)').addClass('active');
			$('#main-menu ul > li:has(ul .active)').removeClass('active');			
	        $('#main-menu ul > li > ul > li:has(.active)').removeClass('active');

		}
	}
}(jQuery));
;
(function ($) {
Drupal.behaviors.stanford_framework = {
	attach: function (context, settings) {

	if ($('#wrap').outerHeight(true) < $(window).height()) {	
		$('#push').css('height', $('#footer').outerHeight(true) + $('#global-footer').outerHeight(true));
		$('#wrap').css('margin-bottom', 0 - $('#push').outerHeight(true) );
			}
		}
	}
}(jQuery));
;
;
jQuery(document).ready(function($) {
	// Bootstrap Carousel
	$('.carousel').attr('id', 'myCarousel');
	$('.carousel .view-content').addClass('carousel-inner');
	$('.carousel .item:nth-child(1)').addClass('active');
	$('.carousel').carousel({
		interval: 8000 // use false to disable auto cycling, or use a number 4000
	});
});;
