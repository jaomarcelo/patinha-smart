const API_URL =
  "https://patinha-smart.onrender.com";

export async function getStatus() {

  const resposta =
    await fetch(
      `${API_URL}/status`
    );

  return resposta.json();
}

export async function getHistorico() {

  const resposta =
    await fetch(
      `${API_URL}/historico`
    );

  return resposta.json();
}