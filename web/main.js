const symbolSelect = document.getElementById("symbol");
const bookBody = document.querySelector("#bookTable tbody");
const tradesBody = document.querySelector("#tradesTable tbody");

async function fetchBook(sym) {
  const res = await fetch(`http://localhost:8080/book/${sym}?depth=10`);
  const j = await res.json();
  bookBody.innerHTML = "";
  j.bids.forEach((lvl) => {
    const row = bookBody.insertRow();
    row.insertCell().textContent = lvl.price.toFixed(2);
    row.insertCell().textContent = lvl.qty;
  });
}

async function fetchTrades(sym) {
  const res = await fetch(`http://localhost:8080/trades/${sym}?limit=10`);
  const arr = await res.json();
  tradesBody.innerHTML = "";
  arr.forEach((t) => {
    const row = tradesBody.insertRow();
    row.insertCell().textContent = t.tradeId;
    row.insertCell().textContent = t.price.toFixed(2);
    row.insertCell().textContent = t.qty;
  });
}

symbolSelect.onchange = () => update();
async function update() {
  const sym = symbolSelect.value;
  await fetchBook(sym);
  await fetchTrades(sym);
}

// Poll every 2 seconds
setInterval(update, 2000);
update();
