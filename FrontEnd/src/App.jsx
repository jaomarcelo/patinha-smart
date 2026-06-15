import React, { useEffect, useState } from "react";

import Header from "./components/Header";
import StatusCard from "./components/StatusCard";
import Alerts from "./components/Alerts";
import HistoryTable from "./components/HistoryTable";
import Footer from "./components/Footer";

import { getStatus, getHistorico } from "./services/api";

function App() {

  const [status, setStatus] = useState({
    racao: 0,
    wifi: false
  });

  const [historico, setHistorico] = useState([]);

  useEffect(() => {

    async function carregarDados() {
      try {
        const dados = await getStatus();
        
        // TRATAMENTO DO ERRO DE MAPEAMENTO:
        // Se a API responder com "peso" (do Arduino), guardamos em "racao".
        // Se responder com "racao", também funciona. Caso contrário, assume 0.
        const dadosTratados = {
          racao: dados.peso !== undefined ? dados.peso : (dados.racao !== undefined ? dados.racao : 0),
          wifi: dados.wifi !== undefined ? dados.wifi : false
        };

        setStatus(dadosTratados);

        const historicoBanco = await getHistorico();

        if (Array.isArray(historicoBanco)) {
          setHistorico(historicoBanco);
        } else {
          console.log("Resposta do histórico:", historicoBanco);
          setHistorico([]);
        }

      } catch (erro) {
        console.log("Erro ao carregar os dados da API:", erro);
      }
    }

    carregarDados();

    // Executa a função de 5 em 5 segundos para atualizar em tempo real
    const intervalo = setInterval(carregarDados, 5000);

    return () => clearInterval(intervalo);

  }, []);

  return (
    <div className="container">
      <Header />

      <div className="grid">
        <StatusCard
          titulo="Ração"
          // Exibe o valor formatado com uma casa decimal e a unidade em gramas
          valor={`${Number(status.racao).toFixed(1)} g`}
          emoji="🍖"
        />

        <StatusCard
          titulo="Wi-Fi"
          valor={status.wifi ? "Conectado" : "Offline"}
          emoji="📶"
        />
      </div>

      {/* O componente de alertas recebe o valor corrigido em gramas */}
      <Alerts racao={status.racao} />

      <HistoryTable historico={historico} />

      <Footer />
    </div>
  );
}

export default App;
