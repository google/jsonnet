var menu_timeout = null;

function set_visible(menu, b)
{
  var category = menu.children[0];
  var dropdown = menu.children[1];
  // category.style['border-radius'] = b && dropdown != null ? '4px 4px 0px 0px' : '4px';
  if (dropdown != null) {
    dropdown.style.visibility = b ? 'visible' : 'hidden';
  }
}

function find_enclosing_menu(el)
{
  while (!el.classList.contains('menu')) {
    el = el.parentNode;
  }
  return el;
}

/* Find the menu above this element and open its popup. */
function menu_open_popup(el)
{
  menu_close_all();
  set_visible(el, true);
}

function menu_leave()
{
  if (menu_timeout != null) {
    window.clearTimeout(menu_timeout);
  }
  menu_timeout = window.setTimeout(menu_close_all, 300);
}

function menu_close_all()
{
  for (let menu of document.getElementsByClassName('menu')) {
    set_visible(menu, false);
  }
  if (menu_timeout != null) {
    window.clearTimeout(menu_timeout);
    menu_timeout = null;
  }
}

document.onclick = menu_close_all();
