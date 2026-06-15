export default function HistoryTable({ historico = [] }) {

  if (!Array.isArray(historico)) {
    return (
      <div className="card">
        <h2>📊 Histórico</h2>
        <p>Nenhum dado disponível.</p>
      </div>
    );
  }

  return (
    <div className="card">
      <h2>📊 Histórico</h2>

      <table>
        <thead>
          <tr>
            <th>Data/Hora</th>
            <th>Ração</th>
          </tr>
        </thead>

        <tbody>
          {historico.map((item, index) => (
            <tr key={item.id || index}>
              <td>{item.data || "-"}</td>
              <td>{item.racao || 0}g</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  );
}
