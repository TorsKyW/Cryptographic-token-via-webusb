import connect from './connect.js';
import pair from './pair.js';
const pairButton = document.querySelector('.pair_button');
const connectButton = document.querySelector('.connect');

connectButton.addEventListener('click', connect);
pairButton.addEventListener('click',pair);
