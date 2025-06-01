import connect from './connect.js';
import pair from './pair.js';
import test from './test.js';
const pairButton = document.querySelector('.pair_button');
const connectButton = document.querySelector('.connect');
const testButton = document.querySelector('.test');

testButton.addEventListener('click',test);
connectButton.addEventListener('click', connect);
pairButton.addEventListener('click',pair);
