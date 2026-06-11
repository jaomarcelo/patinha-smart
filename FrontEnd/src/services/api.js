export async function getStatus() {
  const response = await fetch("http://localhost:8000/status");

  if (!response.ok) {
    throw new Error("Erro ao buscar dados");
  }

  return response.json();
}