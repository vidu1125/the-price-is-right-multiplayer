const API_BASE_URL = process.env.REACT_APP_API_URL || 'http://localhost:5000/api';

export const authService = {
  register: async (email, password, name) => {
    try {
      const response = await fetch(`${API_BASE_URL}/auth/register`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ email, password, name }),
      });

      const data = await response.json();

      if (!response.ok) {
        return {
          success: false,
          error: data.error || 'Registration failed',
        };
      }

      // Store account ID and user info
      if (data.user && data.user.id) {
        localStorage.setItem('account_id', data.user.id);
        localStorage.setItem('user_email', data.user.email);
        localStorage.setItem('user_name', data.profile?.name || name);
      }

      return {
        success: true,
        user: data.user,
        profile: data.profile,
      };
    } catch (error) {
      console.error('Registration error:', error);
      return {
        success: false,
        error: 'Network error',
      };
    }
  },

  login: async (email, password) => {
    try {
      const response = await fetch(`${API_BASE_URL}/auth/login`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ email, password }),
      });

      const data = await response.json();

      if (!response.ok) {
        return {
          success: false,
          error: data.error || 'Login failed',
        };
      }

      // Store account ID and user info
      if (data.user && data.user.id) {
        localStorage.setItem('account_id', data.user.id);
        localStorage.setItem('user_email', data.user.email);
        localStorage.setItem('user_name', data.profile?.name || '');
      }

      return {
        success: true,
        user: data.user,
        profile: data.profile,
      };
    } catch (error) {
      console.error('Login error:', error);
      return {
        success: false,
        error: 'Network error',
      };
    }
  },

  logout: () => {
    localStorage.removeItem('account_id');
    localStorage.removeItem('user_email');
    localStorage.removeItem('user_name');
  },

  isLoggedIn: () => {
    return !!localStorage.getItem('account_id');
  },

  getCurrentUser: () => {
    return {
      id: localStorage.getItem('account_id'),
      email: localStorage.getItem('user_email'),
      name: localStorage.getItem('user_name'),
    };
  },
};
