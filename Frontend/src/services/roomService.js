import axios from 'axios';

const API_URL = process.env.REACT_APP_API_URL || 'http://localhost:5000/api';

// Get account_id from localStorage
const getAccountId = () => {
    return localStorage.getItem('account_id') || '1'; // Default test account
};

export const roomService = {
    createRoom: async (roomData) => {
        try {
            const response = await axios.post(`${API_URL}/room/create`, roomData, {
                headers: {
                    'Content-Type': 'application/json',
                    'X-Account-ID': getAccountId()
                }
            });
            return response.data;
        } catch (error) {
            throw error.response?.data || { error: 'Failed to create room' };
        }
    },

    deleteRoom: async (roomId) => {
        try {
            const response = await axios.delete(`${API_URL}/room/${roomId}`, {
                headers: {
                    'X-Account-ID': getAccountId()
                }
            });
            return response.data;
        } catch (error) {
            throw error.response?.data || { error: 'Failed to delete room' };
        }
    },

    listRooms: async (status = 'waiting', visibility = 'public') => {
        try {
            const response = await axios.get(`${API_URL}/room/list`, {
                params: { status, visibility }
            });
            return response.data;
        } catch (error) {
            throw error.response?.data || { error: 'Failed to list rooms' };
        }
    },

    getRoomDetails: async (roomId) => {
        try {
            const response = await axios.get(`${API_URL}/room/${roomId}`);
            return response.data;
        } catch (error) {
            throw error.response?.data || { error: 'Failed to get room details' };
        }
    }
};